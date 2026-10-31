#include "operationscontroller.h"

#include "archive/archiver.h"
#include "fileops/fileops.h"
#include "foldericons/foldericons.h"
#include "format/format.h"
#include "pathbar/pathbar.h"
#include "trash/trash.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QJSEngine>
#include <sys/stat.h>

namespace {

QString qs(const std::string& text) { return QString::fromStdString(text); }

std::vector<std::string> toStd(const QStringList& list)
{
    std::vector<std::string> out;
    out.reserve(static_cast<std::size_t>(list.size()));
    for (const QString& item : list) out.push_back(item.toStdString());
    return out;
}

QString countText(const QString& verb, qsizetype count)
{
    return count == 1 ? QStringLiteral("%1 1 item").arg(verb) : QStringLiteral("%1 %2 items").arg(verb).arg(count);
}

QString describeFile(const std::string& path)
{
    struct stat info {};
    if (lstat(path.c_str(), &info) != 0) return {};
    return QStringLiteral("%1, modified %2").arg(qs(S_ISDIR(info.st_mode) ? "Folder" : Format::size(static_cast<std::uint64_t>(info.st_size))),
                                                 qs(Format::date(info.st_mtime)));
}

} // namespace

OperationsController* OperationsController::instance()
{
    static OperationsController* controller = new OperationsController(QCoreApplication::instance());
    return controller;
}

OperationsController* OperationsController::create(QQmlEngine*, QJSEngine*)
{
    OperationsController* controller = instance();
    QJSEngine::setObjectOwnership(controller, QJSEngine::CppOwnership);
    return controller;
}

OperationsController::OperationsController(QObject* parent)
    : QAbstractListModel(parent)
{
}

// ---------------------------------------------------------------- the list of running operations

int OperationsController::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_operations.size());
}

QVariant OperationsController::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_operations.size()) return {};
    const Operation& op = *m_operations.at(index.row());
    switch (role) {
    case IdRole: return op.id;
    case TitleRole: return op.title;
    case DetailRole: return op.detail;
    case ProgressRole: return op.progress;
    case PausedRole: return op.paused;
    case CanPauseRole: return op.transfer != nullptr;
    default: return {};
    }
}

QHash<int, QByteArray> OperationsController::roleNames() const
{
    return {{IdRole, "operationId"}, {TitleRole, "title"}, {DetailRole, "detail"}, {ProgressRole, "progress"},
            {PausedRole, "paused"}, {CanPauseRole, "canPause"}};
}

int OperationsController::addOperation(const QString& title)
{
    auto op = std::make_shared<Operation>();
    op->id = m_nextId++;
    op->title = title;
    op->cancelled = std::make_shared<std::atomic<bool>>(false);
    op->job = std::make_unique<BackgroundJob>();
    beginInsertRows({}, m_operations.size(), m_operations.size());
    m_operations.append(op);
    endInsertRows();
    return op->id;
}

int OperationsController::rowOf(int id) const
{
    for (int row = 0; row < m_operations.size(); ++row)
        if (m_operations.at(row)->id == id) return row;
    return -1;
}

void OperationsController::update(int id, const QString& detail, double progress)
{
    const int row = rowOf(id);
    if (row < 0) return;
    m_operations.at(row)->detail = detail;
    m_operations.at(row)->progress = progress;
    emit dataChanged(index(row), index(row), {DetailRole, ProgressRole});
}

void OperationsController::finishOperation(int id)
{
    const int row = rowOf(id);
    if (row < 0) return;
    beginRemoveRows({}, row, row);
    m_operations.removeAt(row);
    endRemoveRows();
    emit operationFinished();
}

void OperationsController::toUi(const BackgroundJob::Handle& handle, std::function<void()> work)
{
    handle.post([this, work = std::move(work)] {
        QMetaObject::invokeMethod(this, [work] { work(); }, Qt::QueuedConnection);
    });
}

void OperationsController::reportProblems(const QString& title, const std::vector<std::string>& problems)
{
    if (problems.empty()) return;
    QStringList lines;
    for (const std::string& problem : problems) lines << qs(problem);
    emit errors(title, lines.join(QLatin1Char('\n')));
}

void OperationsController::pause(int id)
{
    const int row = rowOf(id);
    if (row < 0 || !m_operations.at(row)->transfer) return;
    Operation& op = *m_operations.at(row);
    op.paused = !op.paused;
    op.transfer->setPaused(op.paused);
    emit dataChanged(index(row), index(row), {PausedRole});
}

void OperationsController::cancel(int id)
{
    const int row = rowOf(id);
    if (row < 0) return;
    Operation& op = *m_operations.at(row);
    *op.cancelled = true;
    if (op.transfer) op.transfer->cancel();

    for (int i = m_conflicts.size() - 1; i >= 0; --i) {
        if (m_conflicts.at(i).operation != id) continue;
        m_conflicts.at(i).answer->set_value({ConflictChoice::Cancel, false});
        m_conflicts.removeAt(i);
    }
    emit conflictChanged();
}

// ---------------------------------------------------------------- conflicts

QString OperationsController::conflictName() const
{
    return m_conflicts.isEmpty() ? QString() : qs(PathBar::baseName(m_conflicts.first().target));
}

QString OperationsController::conflictSource() const
{
    return m_conflicts.isEmpty() ? QString() : describeFile(m_conflicts.first().source);
}

QString OperationsController::conflictTarget() const
{
    return m_conflicts.isEmpty() ? QString() : describeFile(m_conflicts.first().target);
}

void OperationsController::showConflict(int id, const std::string& source, const std::string& target,
                                        const std::shared_ptr<std::promise<ConflictAnswer>>& answer)
{
    m_conflicts.append({id, source, target, answer});
    emit conflictChanged();
}

void OperationsController::answerConflict(int choice, bool applyToAll)
{
    if (m_conflicts.isEmpty()) return;
    static const ConflictChoice choices[] = {ConflictChoice::Overwrite, ConflictChoice::Skip, ConflictChoice::KeepBoth, ConflictChoice::Cancel};
    const Conflict conflict = m_conflicts.takeFirst();
    conflict.answer->set_value({choices[qBound(0, choice, 3)], applyToAll});
    emit conflictChanged();
}

// ---------------------------------------------------------------- copy and move

void OperationsController::transfer(TransferKind kind, const QStringList& paths, const QString& destination, UndoStep::Kind undoKind)
{
    if (paths.isEmpty() || PathBar::isVirtual(destination.toStdString())) return;

    const QString verb = kind == TransferKind::Copy ? QStringLiteral("Copying") : QStringLiteral("Moving");
    const int id = addOperation(countText(verb, paths.size()));
    Operation& op = *m_operations.at(rowOf(id));

    Transfer::Events events;
    auto handleSlot = std::make_shared<std::unique_ptr<BackgroundJob::Handle>>();

    events.progress = [this, id, handleSlot](const TransferProgress& p) {
        const double share = p.bytesTotal ? double(p.bytesDone) / double(p.bytesTotal)
                           : p.filesTotal ? double(p.filesDone) / double(p.filesTotal) : 0;
        const QString detail = QStringLiteral("%1 of %2  %3").arg(qs(Format::size(p.bytesDone)), qs(Format::size(p.bytesTotal)), qs(p.currentName));
        if (*handleSlot) toUi(**handleSlot, [this, id, detail, share] { update(id, detail, share); });
    };
    events.conflict = [this, id, handleSlot](const std::string& source, const std::string& target) {
        auto answer = std::make_shared<std::promise<ConflictAnswer>>();
        auto future = answer->get_future();
        toUi(**handleSlot, [this, id, source, target, answer] { showConflict(id, source, target, answer); });
        return future.get();                  // this background thread waits for the user's answer
    };

    auto worker = std::make_shared<Transfer>(kind, toStd(paths), destination.toStdString(), std::move(events));
    op.transfer = worker;
    emit dataChanged(index(rowOf(id)), index(rowOf(id)), {CanPauseRole});

    const QString doneVerb = kind == TransferKind::Copy ? QStringLiteral("Copied") : QStringLiteral("Moved");
    const QString undoName = kind == TransferKind::Copy ? QStringLiteral("Copy") : QStringLiteral("Move");
    op.job->start([this, id, worker, handleSlot, doneVerb, undoName, undoKind](const BackgroundJob::Handle& handle) {
        *handleSlot = std::make_unique<BackgroundJob::Handle>(handle);
        TransferResult result = worker->run();
        toUi(handle, [this, id, result, doneVerb, undoName, undoKind] {
            if (!result.done.empty()) {
                UndoStep step {undoKind, {}, undoName.toStdString()};
                for (const auto& [from, to] : result.done)
                    step.pairs.emplace_back(undoKind == UndoStep::Kind::Moved ? from : std::string(), to);
                m_undo.push(std::move(step));
                emit undoChanged();
            }
            const auto finished = static_cast<qsizetype>(result.done.size() + result.merged);
            if (finished > 0) emit message(countText(doneVerb, finished) + QStringLiteral("."));
            reportProblems(QStringLiteral("Some items were not transferred"), result.errors);
            finishOperation(id);
        });
    });
}

void OperationsController::copy(const QStringList& paths, const QString& destination)
{
    transfer(TransferKind::Copy, paths, destination, UndoStep::Kind::Created);
}

void OperationsController::move(const QStringList& paths, const QString& destination)
{
    transfer(TransferKind::Move, paths, destination, UndoStep::Kind::Moved);
}

void OperationsController::dropUrls(const QList<QUrl>& urls, const QString& destination)
{
    QStringList paths;
    for (const QUrl& url : urls)
        if (url.isLocalFile()) paths << QUrl(url).adjusted(QUrl::StripTrailingSlash).toLocalFile();
    if (paths.isEmpty()) return;

    if (destination.toStdString() == PathBar::kTrashLocation) {
        trash(paths);
        return;
    }
    if (PathBar::isVirtual(destination.toStdString())) return;

    const Qt::KeyboardModifiers keys = QGuiApplication::queryKeyboardModifiers();
    bool moving = FileOps::onSameDevice(paths.first().toStdString(), destination.toStdString());
    if (keys & Qt::ControlModifier) moving = false;
    if (keys & Qt::ShiftModifier) moving = true;

    // only a move can have nothing to do: Ctrl on a drop in the same folder asks for a duplicate
    if (moving) paths.removeIf([&destination](const QString& path) { return FileOps::alreadyIn(path.toStdString(), destination.toStdString()); });
    else paths.removeIf([&destination](const QString& path) { return path == destination; });
    if (paths.isEmpty()) return;

    if (moving) move(paths, destination);
    else copy(paths, destination);
}

// ---------------------------------------------------------------- Trash and delete

void OperationsController::trash(const QStringList& paths)
{
    if (paths.isEmpty()) return;
    const int id = addOperation(countText(QStringLiteral("Moving to Trash:"), paths.size()));
    auto cancelled = m_operations.at(rowOf(id))->cancelled;
    const auto list = toStd(paths);

    m_operations.at(rowOf(id))->job->start([this, id, list, cancelled](const BackgroundJob::Handle& handle) {
        UndoStep step {UndoStep::Kind::Trashed, {}, "Move to Trash"};
        QStringList failed;
        std::string reason;
        for (std::size_t i = 0; i < list.size() && !*cancelled; ++i) {
            std::string trashed;
            const Outcome outcome = Trash::moveToTrash(list[i], &trashed);
            if (outcome.ok) step.pairs.emplace_back(list[i], trashed);
            else {
                failed << qs(list[i]);
                reason = outcome.error;
            }
            const double share = double(i + 1) / double(list.size());
            toUi(handle, [this, id, name = qs(PathBar::baseName(list[i])), share] { update(id, name, share); });
        }
        toUi(handle, [this, id, step, failed, reason] {
            if (!step.pairs.empty()) {
                m_undo.push(step);
                emit undoChanged();
                emit message(countText(QStringLiteral("Moved to Trash:"), static_cast<qsizetype>(step.pairs.size())) + QStringLiteral("."));
            }
            if (!failed.isEmpty()) emit trashFailed(failed, qs(reason));
            finishOperation(id);
        });
    });
}

void OperationsController::deleteForever(const QStringList& paths)
{
    if (paths.isEmpty()) return;
    const int id = addOperation(countText(QStringLiteral("Deleting"), paths.size()));
    auto cancelled = m_operations.at(rowOf(id))->cancelled;
    const auto list = toStd(paths);

    m_operations.at(rowOf(id))->job->start([this, id, list, cancelled](const BackgroundJob::Handle& handle) {
        std::vector<std::string> problems;
        const auto stop = [&cancelled] { return cancelled->load(); };
        for (std::size_t i = 0; i < list.size() && !*cancelled; ++i) {
            const Outcome outcome = Trash::contains(list[i]) ? Trash::remove(list[i]) : FileOps::removeTree(list[i], stop);
            if (!outcome.ok) problems.push_back(outcome.error);
            const double share = double(i + 1) / double(list.size());
            toUi(handle, [this, id, name = qs(PathBar::baseName(list[i])), share] { update(id, name, share); });
        }
        toUi(handle, [this, id, problems] {
            reportProblems(QStringLiteral("Some items were not deleted"), problems);
            finishOperation(id);
        });
    });
}

void OperationsController::restore(const QStringList& paths)
{
    if (paths.isEmpty()) return;
    const int id = addOperation(countText(QStringLiteral("Restoring"), paths.size()));
    const auto list = toStd(paths);
    m_operations.at(rowOf(id))->job->start([this, id, list](const BackgroundJob::Handle& handle) {
        std::vector<std::string> problems;
        for (const std::string& path : list) {
            const Outcome outcome = Trash::restore(path);
            if (!outcome.ok) problems.push_back(outcome.error);
        }
        toUi(handle, [this, id, problems] {
            reportProblems(QStringLiteral("Some items were not restored"), problems);
            finishOperation(id);
        });
    });
}

void OperationsController::restoreAll()
{
    QStringList paths;
    for (const TrashItem& item : Trash::list()) paths << qs(item.path);
    restore(paths);
}

void OperationsController::emptyTrash()
{
    const int id = addOperation(QStringLiteral("Emptying the Trash"));
    m_operations.at(rowOf(id))->job->start([this, id](const BackgroundJob::Handle& handle) {
        const Outcome outcome = Trash::empty();
        toUi(handle, [this, id, outcome] {
            if (!outcome.ok) emit errors(QStringLiteral("The Trash could not be emptied"), qs(outcome.error));
            finishOperation(id);
        });
    });
}

// ---------------------------------------------------------------- archives and links

void OperationsController::compress(const QStringList& paths, const QString& archivePath)
{
    if (paths.isEmpty()) return;
    const int id = addOperation(QStringLiteral("Compressing into %1").arg(qs(PathBar::baseName(archivePath.toStdString()))));
    auto cancelled = m_operations.at(rowOf(id))->cancelled;
    const auto list = toStd(paths);
    const std::string archive = archivePath.toStdString();

    m_operations.at(rowOf(id))->job->start([this, id, list, archive, cancelled](const BackgroundJob::Handle& handle) {
        const Outcome outcome = Archiver::compress(list, archive, [&cancelled] { return cancelled->load(); });
        toUi(handle, [this, id, outcome, archive] {
            if (outcome.ok) {
                m_undo.push({UndoStep::Kind::Created, {{"", archive}}, "Compress"});
                emit undoChanged();
            } else if (outcome.error != "Cancelled.") {
                emit errors(QStringLiteral("The archive was not created"), qs(outcome.error));
            }
            finishOperation(id);
        });
    });
}

void OperationsController::extract(const QStringList& archives)
{
    for (const QString& archive : archives) {
        const std::string path = archive.toStdString();
        const int id = addOperation(QStringLiteral("Extracting %1").arg(qs(PathBar::baseName(path))));
        auto cancelled = m_operations.at(rowOf(id))->cancelled;

        m_operations.at(rowOf(id))->job->start([this, id, path, cancelled](const BackgroundJob::Handle& handle) {
            // always into a new folder, so an archive can't scatter over or overwrite what is there
            const std::string folder = FileOps::uniquePath(PathBar::parent(path) + "/" + Archiver::withoutExtension(PathBar::baseName(path)));
            Outcome outcome = FileOps::createFolder(folder);
            if (outcome.ok) outcome = Archiver::extract(path, folder, [&cancelled] { return cancelled->load(); });
            if (!outcome.ok) FileOps::removeTree(folder, {});
            toUi(handle, [this, id, outcome, folder] {
                if (outcome.ok) {
                    m_undo.push({UndoStep::Kind::Created, {{"", folder}}, "Extract"});
                    emit undoChanged();
                } else if (outcome.error != "Cancelled.") {
                    emit errors(QStringLiteral("The archive was not extracted"), qs(outcome.error));
                }
                finishOperation(id);
            });
        });
    }
}

void OperationsController::createLinks(const QStringList& paths)
{
    UndoStep step {UndoStep::Kind::Created, {}, "Create Link"};
    std::vector<std::string> problems;
    for (const QString& path : paths) {
        const std::string target = path.toStdString();
        const std::string link = FileOps::uniquePath(target + " link");
        const Outcome outcome = FileOps::createLink(target, link);
        if (outcome.ok) step.pairs.emplace_back("", link);
        else problems.push_back(outcome.error);
    }
    if (!step.pairs.empty()) {
        m_undo.push(std::move(step));
        emit undoChanged();
    }
    reportProblems(QStringLiteral("Some links were not created"), problems);
}

// ---------------------------------------------------------------- small changes

QString OperationsController::rename(const QString& path, const QString& newName)
{
    const std::string name = newName.toStdString();
    if (const std::string problem = FileOps::nameProblem(name); !problem.empty()) return qs(problem);

    const std::string from = path.toStdString();
    const std::string folder = PathBar::parent(from);
    const std::string to = folder == "/" ? "/" + name : folder + "/" + name;
    const Outcome outcome = FileOps::rename(from, to);
    if (!outcome.ok) return qs(outcome.error);
    if (from != to) {
        m_undo.push({UndoStep::Kind::Renamed, {{from, to}}, "Rename"});
        emit undoChanged();
    }
    return {};
}

QString OperationsController::newFolder(const QString& folder, const QString& name)
{
    const std::string clean = name.toStdString();
    if (const std::string problem = FileOps::nameProblem(clean); !problem.empty()) return qs(problem);
    const std::string path = folder.toStdString() + "/" + clean;
    const Outcome outcome = FileOps::createFolder(path);
    if (!outcome.ok) return qs(outcome.error);
    m_undo.push({UndoStep::Kind::Created, {{"", path}}, "New Folder"});
    emit undoChanged();
    return {};
}

QString OperationsController::newFile(const QString& folder, const QString& name)
{
    const std::string clean = name.toStdString();
    if (const std::string problem = FileOps::nameProblem(clean); !problem.empty()) return qs(problem);
    const std::string path = folder.toStdString() + "/" + clean;
    const Outcome outcome = FileOps::createFile(path);
    if (!outcome.ok) return qs(outcome.error);
    m_undo.push({UndoStep::Kind::Created, {{"", path}}, "New File"});
    emit undoChanged();
    return {};
}

QString OperationsController::nameProblem(const QString& name) const
{
    return qs(FileOps::nameProblem(name.toStdString()));
}

QString OperationsController::setFolderIcon(const QStringList& paths, const QString& icon, const QString& color)
{
    const std::string value = FolderIcons::make(icon.toStdString(), color.toStdString());
    QString failure;
    for (const QString& path : paths) {
        const Outcome outcome = FolderIcons::write(path.toStdString(), value);
        if (!outcome.ok) failure = qs(outcome.error);
    }
    return failure;
}

QVariantMap OperationsController::folderIcon(const QString& path) const
{
    const std::string value = FolderIcons::read(path.toStdString());
    return {{"icon", qs(FolderIcons::iconOf(value))}, {"color", qs(FolderIcons::colorOf(value))}};
}

QString OperationsController::freeName(const QString& folder, const QString& name) const
{
    return qs(PathBar::baseName(FileOps::uniquePath(folder.toStdString() + "/" + name.toStdString())));
}

bool OperationsController::isArchive(const QString& name) const
{
    return Archiver::isArchive(name.toStdString());
}

QString OperationsController::withoutExtension(const QString& name) const
{
    return qs(Archiver::withoutExtension(name.toStdString()));
}

// ---------------------------------------------------------------- undo

QString OperationsController::undoText() const
{
    return m_undo.canUndo() ? QStringLiteral("Undo %1").arg(qs(m_undo.nextDescription())) : QStringLiteral("Undo");
}

void OperationsController::undo()
{
    if (!m_undo.canUndo()) return;
    UndoStep step = m_undo.take();
    emit undoChanged();

    const int id = addOperation(QStringLiteral("Undoing %1").arg(qs(step.description)));
    m_operations.at(rowOf(id))->job->start([this, id, step](const BackgroundJob::Handle& handle) {
        std::vector<std::string> problems;
        for (const auto& [first, second] : step.pairs) {
            Outcome outcome;
            switch (step.kind) {
            case UndoStep::Kind::Renamed:
                outcome = FileOps::rename(second, first);
                break;
            case UndoStep::Kind::Moved: {
                outcome = FileOps::rename(second, first);
                if (!outcome.ok && !FileOps::onSameDevice(second, PathBar::parent(first))) {
                    Transfer back(TransferKind::Move, {second}, PathBar::parent(first), {});   // no conflict handler: stops rather than replaces
                    const TransferResult result = back.run();
                    outcome = result.done.empty() ? Outcome {false, result.errors.empty() ? "Cancelled." : result.errors.front()} : Outcome {};
                }
                break;
            }
            case UndoStep::Kind::Trashed:
                outcome = Trash::restore(second);
                break;
            case UndoStep::Kind::Created:
                outcome = Trash::moveToTrash(second);     // never deleted for good: the Trash keeps it safe
                break;
            }
            if (!outcome.ok) problems.push_back(outcome.error);
        }
        toUi(handle, [this, id, problems] {
            reportProblems(QStringLiteral("Undo was not complete"), problems);
            finishOperation(id);
        });
    });
}
