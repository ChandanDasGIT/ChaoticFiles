#include "placescontroller.h"

#include "drives/drives.h"
#include "foldericons/foldericons.h"
#include "network/networkshares.h"
#include "pathbar/pathbar.h"
#include "places/places.h"
#include "trash/trash.h"

namespace {

constexpr int kDriveCheckMs = 4000;

QVariantList drivesList(const std::vector<Place>& mountedDrives, const std::vector<DriveDevice>& others)
{
    QVariantList list;
    for (const Place& place : mountedDrives) {
        list.append(QVariantMap {
            {"name", QString::fromStdString(place.name)},
            {"path", QString::fromStdString(place.path)},
            {"kind", QStringLiteral("drive")},
            {"device", QString::fromStdString(place.device)},
            {"mounted", true},
        });
    }
    for (const DriveDevice& drive : others) {
        list.append(QVariantMap {
            {"name", QString::fromStdString(drive.displayName())},
            {"path", QString()},
            {"kind", QStringLiteral("drive")},
            {"device", QString::fromStdString(drive.device)},
            {"mounted", false},
        });
    }
    return list;
}

QVariantList sharesList(const std::vector<NetworkShare>& shares)
{
    QVariantList list;
    for (const NetworkShare& share : shares) {
        list.append(QVariantMap {
            {"name", QString::fromStdString(share.name)},
            {"path", QString::fromStdString(share.path)},
            {"kind", QStringLiteral("network")},
            {"uri", QString::fromStdString(share.uri)},
        });
    }
    return list;
}

QVariantList toList(const std::vector<Place>& places)
{
    QVariantList list;
    list.reserve(static_cast<qsizetype>(places.size()));
    for (const Place& place : places) {
        list.append(QVariantMap {
            {"name", QString::fromStdString(place.name)},
            {"path", QString::fromStdString(place.path)},
            {"kind", QString::fromStdString(place.kind)},
        });
    }
    return list;
}

} // namespace

PlacesController::PlacesController(QObject* parent)
    : QObject(parent), m_places(toList(Places::userPlaces())), m_drives(drivesList(Places::drives(), {}))
{
    Trash::ensureExists();
    connect(&m_timer, &QTimer::timeout, this, &PlacesController::updateDrives);
    m_timer.start(kDriveCheckMs);
    updateDrives();
}

QVariantList PlacesController::bookmarks() const
{
    QVariantList list;
    for (const QString& path : m_settings.value("places/bookmarks").toStringList()) {
        const std::string icon = FolderIcons::read(path.toStdString());
        list.append(QVariantMap {
            {"name", QString::fromStdString(PathBar::tabTitle(path.toStdString()))},
            {"path", path},
            {"kind", QStringLiteral("bookmark")},
            {"iconKey", QString::fromStdString(FolderIcons::iconOf(icon))},
            {"colorKey", QString::fromStdString(FolderIcons::colorOf(icon))},
        });
    }
    return list;
}

void PlacesController::addBookmark(const QString& path)
{
    QStringList paths = m_settings.value("places/bookmarks").toStringList();
    if (paths.contains(path)) return;
    paths << path;
    m_settings.setValue("places/bookmarks", paths);
    emit bookmarksChanged();
}

void PlacesController::removeBookmark(const QString& path)
{
    QStringList paths = m_settings.value("places/bookmarks").toStringList();
    if (paths.removeAll(path) == 0) return;
    m_settings.setValue("places/bookmarks", paths);
    emit bookmarksChanged();
}

bool PlacesController::isBookmarked(const QString& path) const
{
    return m_settings.value("places/bookmarks").toStringList().contains(path);
}

void PlacesController::updateDrives()
{
    m_scan.start([this](const BackgroundJob::Handle& handle) {
        QVariantList drives = drivesList(Places::drives(), Drives::unmounted());
        QVariantList shares = sharesList(NetworkShares::list());
        handle.post([this, drives, shares] {
            QMetaObject::invokeMethod(this, [this, drives, shares] {
                if (drives == m_drives && shares == m_shares) return;
                m_drives = drives;
                m_shares = shares;
                emit drivesChanged();
            }, Qt::QueuedConnection);
        });
    });
}

void PlacesController::mount(const QString& device)
{
    m_action.start([this, device](const BackgroundJob::Handle& handle) {
        std::string mountPoint;
        const Outcome outcome = Drives::mount(device.toStdString(), &mountPoint);
        handle.post([this, outcome, mountPoint] {
            QMetaObject::invokeMethod(this, [this, outcome, mountPoint] {
                updateDrives();
                if (outcome.ok) emit mounted(QString::fromStdString(mountPoint));
                else emit problem(QStringLiteral("The drive was not mounted"), QString::fromStdString(outcome.error));
            }, Qt::QueuedConnection);
        });
    });
}

void PlacesController::unmount(const QString& device)
{
    m_action.start([this, device](const BackgroundJob::Handle& handle) {
        const Outcome outcome = Drives::unmount(device.toStdString());
        handle.post([this, outcome] {
            QMetaObject::invokeMethod(this, [this, outcome] {
                updateDrives();
                if (!outcome.ok) emit problem(QStringLiteral("The drive was not unmounted"), QString::fromStdString(outcome.error));
            }, Qt::QueuedConnection);
        });
    });
}

void PlacesController::connectShare(const QString& address, const QString& user, const QString& password)
{
    const std::string uri = NetworkShares::normalizeAddress(address.toStdString());
    m_action.start([this, uri, user, password](const BackgroundJob::Handle& handle) {
        const Outcome outcome = NetworkShares::connect(uri, user.toStdString(), password.toStdString(),
                                                       [&handle] { return handle.cancelled(); });
        // the share's folder is named by gvfs; find it by its address
        std::string path;
        for (const NetworkShare& share : NetworkShares::list())
            if (share.uri.rfind(uri, 0) == 0 || uri.rfind(share.uri, 0) == 0) path = share.path;
        handle.post([this, outcome, path] {
            QMetaObject::invokeMethod(this, [this, outcome, path] {
                updateDrives();
                emit connectFinished(outcome.ok ? QString() : QString::fromStdString(outcome.error));
                if (outcome.ok && !path.empty()) emit mounted(QString::fromStdString(path));
            }, Qt::QueuedConnection);
        });
    });
}

void PlacesController::disconnectShare(const QString& uri)
{
    m_action.start([this, uri](const BackgroundJob::Handle& handle) {
        const Outcome outcome = NetworkShares::disconnect(uri.toStdString());
        handle.post([this, outcome] {
            QMetaObject::invokeMethod(this, [this, outcome] {
                updateDrives();
                if (!outcome.ok) emit problem(QStringLiteral("The connection was not closed"), QString::fromStdString(outcome.error));
            }, Qt::QueuedConnection);
        });
    });
}
