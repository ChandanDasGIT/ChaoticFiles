#include "foldersession.h"

#include "format/format.h"
#include "pathbar/pathbar.h"
#include "trash/trash.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QUrl>
#include <unordered_set>

namespace {

constexpr int kTypeAheadPauseMs = 1000;

QString qs(const std::string& text) { return QString::fromStdString(text); }

// when moving from /a/b/c to /a, the row to highlight in /a is "b"
std::string childOnTheWay(const std::string& from, const std::string& to)
{
    const std::string prefix = to == "/" ? "/" : to + "/";
    if (from.size() <= prefix.size() || from.rfind(prefix, 0) != 0) return {};
    const auto slash = from.find('/', prefix.size());
    return from.substr(prefix.size(), slash == std::string::npos ? std::string::npos : slash - prefix.size());
}

} // namespace

FolderSession::FolderSession(std::shared_ptr<Tab> tab, bool showHidden, QObject* parent)
    : QObject(parent), m_tab(std::move(tab)), m_model(&m_tab->view)
{
    m_tab->view.setShowHidden(showHidden);
    m_typedTimer.start();
}

FolderSession::~FolderSession()
{
    stop();
}

void FolderSession::stop()
{
    ++m_generation;
    m_lister.cancel();
    m_watcher.stop();
}

QString FolderSession::path() const { return qs(m_tab->history.current()); }

bool FolderSession::canGoUp() const
{
    const std::string& path = m_tab->history.current();
    return path != "/" && !PathBar::isVirtual(path);
}

bool FolderSession::isTrash() const
{
    return m_tab->history.current() == PathBar::kTrashLocation;
}

bool FolderSession::canWrite() const
{
    return !PathBar::isVirtual(m_tab->history.current());
}

void FolderSession::setCutPaths(const QStringList& paths)
{
    std::unordered_set<std::string> cut;
    for (const QString& path : paths) cut.insert(path.toStdString());
    m_tab->view.setCutPaths(std::move(cut));
    m_model.selectionChanged();
}

QString FolderSession::title() const { return qs(PathBar::tabTitle(m_tab->history.current())); }

QString FolderSession::itemsText() const { return qs(Format::itemCount(static_cast<std::size_t>(itemCount()))); }

QString FolderSession::selectionText() const
{
    const FolderView& view = m_tab->view;
    if (view.selectedCount() == 0) return {};
    QString text = QStringLiteral("%1 selected").arg(view.selectedCount());
    if (const std::uint64_t size = view.selectedSize()) text += QStringLiteral(" (%1)").arg(qs(Format::size(size)));
    return text;
}

void FolderSession::setShowHidden(bool show)
{
    emit aboutToRefresh();
    m_model.beginChange();
    m_tab->view.setShowHidden(show);
    m_model.endChange();
    emit refreshed();
    emit stateChanged();
    emit selectionChanged();
}

void FolderSession::setActive(bool active)
{
    m_active = active;
    if (active && (!m_loaded || m_stale)) load({});
}

// ---------------------------------------------------------------- reading the folder

void FolderSession::load(const std::string& focusName)
{
    const quint64 generation = ++m_generation;
    m_stale = false;
    m_loading = true;
    emit stateChanged();

    const std::string path = m_tab->history.current();
    m_lister.start(path, m_tab->view.sort(), [this, generation, focusName](ListResult result) {
        // the lister's thread: the work moves to the UI thread, and is dropped if this object is gone
        auto shared = std::make_shared<ListResult>(std::move(result));
        QMetaObject::invokeMethod(this, [this, shared, generation, focusName] {
            finishLoad(shared, generation, focusName);
        }, Qt::QueuedConnection);
    });
}

void FolderSession::finishLoad(const std::shared_ptr<ListResult>& result, quint64 generation, const std::string& focusName)
{
    if (generation != m_generation) return;

    m_loaded = true;
    m_loading = false;
    m_diskText = result->disk.known
        ? QStringLiteral("%1 free of %2").arg(qs(Format::size(result->disk.available)), qs(Format::size(result->disk.total)))
        : QString();

    const bool sameFolder = result->path == m_shownPath;
    FolderView& view = m_tab->view;

    if (!result->ok) {
        m_error = qs(result->error);
        m_model.beginChange();
        view.clear();
        m_model.endChange();
        m_shownPath = result->path;
    } else {
        const bool unchanged = sameFolder && m_error.isEmpty() && view.matches(result->entries, result->sort);
        m_error.clear();
        if (unchanged) {
            if (!focusName.empty()) view.selectName(focusName);
            m_model.selectionChanged();
        } else {
            if (sameFolder) emit aboutToRefresh();
            m_model.beginChange();
            if (!sameFolder) view.clear();
            view.setEntries(std::move(result->entries), result->sort);
            if (!focusName.empty()) view.selectName(focusName);
            m_model.endChange();
            if (sameFolder) emit refreshed();
        }
        m_shownPath = result->path;
    }

    if (m_watchedPath != result->path || !result->ok) {
        m_watchedPath = result->ok ? result->path : std::string();
        if (result->ok) {
            const std::string real = result->path == PathBar::kTrashLocation ? Trash::filesDirectory() : result->path;
            m_watcher.watch(real, [this] {
                QMetaObject::invokeMethod(this, [this] { folderChanged(); }, Qt::QueuedConnection);
            });
        } else {
            m_watcher.stop();
        }
    }

    emit stateChanged();
    emit selectionChanged();
}

void FolderSession::folderChanged()
{
    if (m_active) load({});
    else m_stale = true;
}

// ---------------------------------------------------------------- moving around

void FolderSession::changeLocation(const std::string& oldPath)
{
    emit locationChanged();
    load(childOnTheWay(oldPath, m_tab->history.current()));
}

void FolderSession::navigate(const QString& path)
{
    const std::string oldPath = m_tab->history.current();
    const std::string target = PathBar::normalize(path.toStdString());
    if (target == oldPath) {
        refresh();
        return;
    }
    m_tab->history.visit(target);
    changeLocation(oldPath);
}

void FolderSession::goBack()
{
    const std::string oldPath = m_tab->history.current();
    if (m_tab->history.goBack()) changeLocation(oldPath);
}

void FolderSession::goForward()
{
    const std::string oldPath = m_tab->history.current();
    if (m_tab->history.goForward()) changeLocation(oldPath);
}

void FolderSession::goUp()
{
    if (canGoUp()) navigate(qs(PathBar::parent(m_tab->history.current())));
}

void FolderSession::refresh()
{
    load({});
}

void FolderSession::refreshAndSelect(const QString& name)
{
    load(name.toStdString());
}

// ---------------------------------------------------------------- rows

void FolderSession::openRow(int row)
{
    if (row < 0 || row >= m_tab->view.count()) return;
    const Entry& entry = m_tab->view.at(row);

    if (entry.brokenLink) {
        emit message(QStringLiteral("\"%1\" is a broken link.").arg(qs(entry.name)));
    } else if (entry.isDirectory()) {
        navigate(qs(entry.path));
    } else if (!QDesktopServices::openUrl(QUrl::fromLocalFile(qs(entry.path)))) {
        emit message(QStringLiteral("No app could open \"%1\".").arg(qs(entry.name)));
    }
}

void FolderSession::openCurrent()
{
    openRow(currentRow());
}

void FolderSession::selectionUpdated()
{
    m_model.selectionChanged();
    emit selectionChanged();
}

void FolderSession::click(int row, int modifiers)
{
    ClickMode mode = ClickMode::Replace;
    if (modifiers & Qt::ShiftModifier) mode = ClickMode::Extend;
    else if (modifiers & Qt::ControlModifier) mode = ClickMode::Toggle;
    m_tab->view.click(row, mode);
    selectionUpdated();
}

void FolderSession::contextClick(int row)
{
    if (row < 0 || row >= m_tab->view.count() || m_tab->view.isSelected(row)) return;
    m_tab->view.click(row, ClickMode::Replace);
    selectionUpdated();
}

void FolderSession::moveCurrent(int delta, bool extend)
{
    moveCurrentTo(currentRow() < 0 ? (delta > 0 ? 0 : itemCount() - 1) : currentRow() + delta, extend);
}

void FolderSession::moveCurrentTo(int row, bool extend)
{
    m_tab->view.moveCurrent(row, extend);
    selectionUpdated();
}

void FolderSession::selectAll()
{
    m_tab->view.selectAll();
    selectionUpdated();
}

void FolderSession::clearSelection()
{
    m_tab->view.clearSelection();
    selectionUpdated();
}

void FolderSession::typeAhead(const QString& text)
{
    if (m_typedTimer.restart() > kTypeAheadPauseMs) m_typed.clear();
    m_typed += text;

    // a first letter moves on to the next match; more letters refine from where the list is now
    const int current = currentRow();
    const int from = m_typed.size() == 1 ? current + 1 : qMax(current, 0);
    const int row = m_tab->view.findPrefix(m_typed.toStdString(), from >= itemCount() ? 0 : from);
    if (row >= 0) moveCurrentTo(row, false);
}

void FolderSession::sortBy(int key)
{
    SortSettings sort = m_tab->view.sort();
    if (static_cast<int>(sort.key) == key) {
        sort.ascending = !sort.ascending;
    } else {
        sort.key = static_cast<SortKey>(key);
        sort.ascending = sort.key == SortKey::Name || sort.key == SortKey::Type;
    }
    emit aboutToRefresh();
    m_model.beginChange();
    m_tab->view.setSort(sort);
    m_model.endChange();
    emit refreshed();
    emit sortChanged();
    emit selectionChanged();
}

QStringList FolderSession::selectedPaths() const
{
    QStringList paths;
    for (const std::string& selected : m_tab->view.selectedPaths()) paths << qs(selected);
    if (paths.isEmpty()) paths << path();
    return paths;
}

QStringList FolderSession::selectedItems() const
{
    QStringList paths;
    for (const std::string& selected : m_tab->view.selectedPaths()) paths << qs(selected);
    return paths;
}

QString FolderSession::selectedUrlText() const
{
    QStringList urls;
    for (const std::string& selected : m_tab->view.selectedPaths())
        urls << QString::fromLatin1(QUrl::fromLocalFile(qs(selected)).toEncoded());
    return urls.join(QStringLiteral("\r\n"));
}

void FolderSession::copyPaths()
{
    const QStringList paths = selectedPaths();
    QGuiApplication::clipboard()->setText(paths.join(QLatin1Char('\n')));
    emit message(paths.size() == 1 ? QStringLiteral("Path copied.") : QStringLiteral("%1 paths copied.").arg(paths.size()));
}
