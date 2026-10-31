#include "diskusagecontroller.h"

#include "format/format.h"
#include "pathbar/pathbar.h"

#include <QCoreApplication>
#include <QJSEngine>
#include <QVariantMap>
#include <memory>

DiskUsageController* DiskUsageController::instance()
{
    static DiskUsageController* controller = new DiskUsageController(QCoreApplication::instance());
    return controller;
}

DiskUsageController* DiskUsageController::create(QQmlEngine*, QJSEngine*)
{
    DiskUsageController* controller = instance();
    QJSEngine::setObjectOwnership(controller, QJSEngine::CppOwnership);
    return controller;
}

DiskUsageController::DiskUsageController(QObject* parent)
    : QObject(parent)
{
}

bool DiskUsageController::canGoUp() const
{
    return m_folder != QStringLiteral("/");
}

void DiskUsageController::show(const QString& folder)
{
    m_open = true;
    emit openChanged();
    openFolder(folder);
}

void DiskUsageController::goUp()
{
    if (canGoUp()) openFolder(QString::fromStdString(PathBar::parent(m_folder.toStdString())));
}

void DiskUsageController::close()
{
    ++m_generation;
    m_job.cancel();
    m_scanning = false;
    m_items.clear();
    m_open = false;
    emit openChanged();
    emit reportChanged();
}

void DiskUsageController::openFolder(const QString& folder)
{
    const quint64 generation = ++m_generation;
    m_folder = folder;
    m_items.clear();
    m_summary = QStringLiteral("Adding up…");
    m_scanning = true;
    emit reportChanged();

    m_job.start([this, generation, path = folder.toStdString()](const BackgroundJob::Handle& handle) {
        const auto toUi = [this, &handle, generation](const UsageReport& report) {
            auto shared = std::make_shared<UsageReport>(report);
            handle.post([this, generation, shared] {
                QMetaObject::invokeMethod(this, [this, generation, shared] {
                    if (generation == m_generation) apply(*shared);
                }, Qt::QueuedConnection);
            });
        };
        const UsageReport report = scanUsage(path, toUi, [&handle] { return handle.cancelled(); });
        if (report.finished) toUi(report);
    });
}

void DiskUsageController::apply(const UsageReport& report)
{
    const std::uint64_t biggest = report.items.empty() ? 0 : report.items.front().bytes;
    m_items.clear();
    for (const UsageItem& item : report.items) {
        m_items.append(QVariantMap {
            {"name", QString::fromStdString(item.name)},
            {"path", QString::fromStdString(item.path)},
            {"sizeText", QString::fromStdString(Format::size(item.bytes))},
            {"share", biggest ? double(item.bytes) / double(biggest) : 0.0},    // the longest bar is the biggest item
            {"isFolder", item.folder},
        });
    }

    const QString total = QString::fromStdString(Format::size(report.total));
    m_scanning = !report.finished;
    if (!report.finished) {
        m_summary = QStringLiteral("%1 so far. Adding up %2…").arg(total, QString::fromStdString(report.current));
    } else {
        m_summary = QStringLiteral("%1 on the disk").arg(total);
        if (report.hiddenItems) m_summary += QStringLiteral(". The %1 biggest items are shown").arg(report.items.size());
        if (report.unreadable) m_summary += QStringLiteral(". %1 could not be read").arg(report.unreadable);
    }
    emit reportChanged();
}
