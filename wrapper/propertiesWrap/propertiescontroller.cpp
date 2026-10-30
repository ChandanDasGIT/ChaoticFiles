#include "propertiescontroller.h"

#include "contentinfo/contentinfo.h"
#include "folderscan/folderscan.h"
#include "format/format.h"
#include "hashing/hashing.h"
#include "pathbar/pathbar.h"
#include "properties/properties.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QVariantMap>
#include <sys/stat.h>

namespace {

QString qs(const std::string& text) { return QString::fromStdString(text); }

} // namespace

PropertiesController::PropertiesController(QObject* parent)
    : QObject(parent)
{
}

QVariantList PropertiesController::sections() const
{
    QVariantList list;
    for (const PropertySection& section : m_sections) {
        QVariantList rows;
        for (const PropertyRow& row : section.rows)
            rows.append(QVariantMap {{"label", qs(row.label)}, {"value", qs(row.value)}});
        list.append(QVariantMap {{"title", qs(section.title)}, {"rows", rows}});
    }
    return list;
}

// a background thread hands work to the UI thread; it is dropped if the window has moved on since
void PropertiesController::toUi(const BackgroundJob::Handle& handle, quint64 generation, std::function<void()> work)
{
    handle.post([this, generation, work = std::move(work)] {
        QMetaObject::invokeMethod(this, [this, generation, work] {
            if (generation == m_generation) work();
        }, Qt::QueuedConnection);
    });
}

void PropertiesController::setSection(PropertySection section)
{
    for (PropertySection& existing : m_sections) {
        if (existing.title == section.title) {
            existing = std::move(section);
            emit sectionsChanged();
            return;
        }
    }
    m_sections.push_back(std::move(section));
    emit sectionsChanged();
}

void PropertiesController::show(const QStringList& paths)
{
    if (paths.isEmpty()) return;
    close();
    const quint64 generation = ++m_generation;

    m_open = true;
    m_title = paths.size() == 1 ? qs(PathBar::tabTitle(paths.first().toStdString()))
                                : QStringLiteral("%1 items").arg(paths.size());
    m_path = paths.first();
    m_sections.clear();
    m_status = QStringLiteral("Reading…");
    m_canHash = false;
    m_hashes.clear();
    m_hashError.clear();
    m_hashProgress = 0;
    emit openChanged();
    emit sectionsChanged();
    emit hashesChanged();

    if (paths.size() > 1) {
        std::vector<std::string> list;
        for (const QString& path : paths) list.push_back(path.toStdString());
        m_job.start([this, list, generation](const BackgroundJob::Handle& handle) {
            auto sections = Properties::collectSelection(list);
            toUi(handle, generation, [this, sections] {
                m_sections = sections;
                m_status.clear();
                emit sectionsChanged();
            });
        });
        return;
    }
    collect(paths.first().toStdString());
}

void PropertiesController::collect(const std::string& path)
{
    const quint64 generation = m_generation;
    m_job.start([this, path, generation](const BackgroundJob::Handle& handle) {
        const auto cancelled = [&handle] { return handle.cancelled(); };

        auto sections = Properties::collect(path);
        struct stat info {};
        const bool found = lstat(path.c_str(), &info) == 0;
        const bool regular = found && S_ISREG(info.st_mode);
        const bool folder = found && S_ISDIR(info.st_mode);

        toUi(handle, generation, [this, sections, regular] {
            m_sections = sections;
            m_canHash = regular;
            emit sectionsChanged();
        });

        if (regular) {
            auto rows = ContentInfo::collect(path, cancelled);
            toUi(handle, generation, [this, rows] { setSection({"Content", rows}); });
        } else if (folder) {
            const FolderScan scan = scanFolder(path, [&](const FolderScan& soFar) {
                toUi(handle, generation, [this, section = Properties::folderContents(soFar)] { setSection(section); });
            }, cancelled);
            toUi(handle, generation, [this, section = Properties::folderContents(scan)] { setSection(section); });
        }
        toUi(handle, generation, [this] {
            m_status.clear();
            emit sectionsChanged();
        });
    });
}

void PropertiesController::close()
{
    ++m_generation;
    m_job.cancel();
    m_hashJob.cancel();
    m_hashing = false;
    if (!m_open) return;
    m_open = false;
    emit openChanged();
    emit hashesChanged();
}

void PropertiesController::startHashes()
{
    if (m_hashing || !m_canHash) return;
    m_hashing = true;
    m_hashProgress = 0;
    m_hashes.clear();
    m_hashError.clear();
    emit hashesChanged();

    const quint64 generation = m_generation;
    const std::string path = m_path.toStdString();
    m_hashJob.start([this, path, generation](const BackgroundJob::Handle& handle) {
        std::uint64_t reported = 0;
        std::string error;
        const auto results = Hashing::hashFile(path, [&](std::uint64_t done, std::uint64_t total) {
            if (done - reported < total / 100 + 1) return;
            reported = done;
            const double share = total ? double(done) / double(total) : 1.0;
            toUi(handle, generation, [this, share] {
                m_hashProgress = share;
                emit hashesChanged();
            });
        }, [&handle] { return handle.cancelled(); }, &error);

        toUi(handle, generation, [this, results, error] {
            m_hashing = false;
            m_hashProgress = 1;
            m_hashError = qs(error);
            for (const HashResult& result : results)
                m_hashes.append(QVariantMap {{"label", qs(result.name)}, {"value", qs(result.value)}});
            emit hashesChanged();
        });
    });
}

void PropertiesController::cancelHashes()
{
    m_hashJob.cancel();
    m_hashing = false;
    m_hashProgress = 0;
    emit hashesChanged();
}

void PropertiesController::copyText(const QString& text)
{
    QGuiApplication::clipboard()->setText(text);
}

void PropertiesController::copyAll()
{
    QString text = m_title + QLatin1Char('\n');
    for (const PropertySection& section : m_sections) {
        text += QStringLiteral("\n[%1]\n").arg(qs(section.title));
        for (const PropertyRow& row : section.rows)
            text += QStringLiteral("%1: %2\n").arg(qs(row.label), qs(row.value));
    }
    if (!m_hashes.isEmpty()) {
        text += QStringLiteral("\n[Hashes]\n");
        for (const QVariant& hash : m_hashes) {
            const QVariantMap row = hash.toMap();
            text += QStringLiteral("%1: %2\n").arg(row["label"].toString(), row["value"].toString());
        }
    }
    QGuiApplication::clipboard()->setText(text);
}
