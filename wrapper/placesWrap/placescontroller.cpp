#include "placescontroller.h"

#include "pathbar/pathbar.h"
#include "places/places.h"
#include "trash/trash.h"

namespace {

constexpr int kDriveCheckMs = 3000;

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
    : QObject(parent), m_places(toList(Places::userPlaces())), m_drives(toList(Places::drives()))
{
    Trash::ensureExists();
    connect(&m_timer, &QTimer::timeout, this, &PlacesController::updateDrives);
    m_timer.start(kDriveCheckMs);
}

QVariantList PlacesController::bookmarks() const
{
    QVariantList list;
    for (const QString& path : m_settings.value("places/bookmarks").toStringList()) {
        list.append(QVariantMap {
            {"name", QString::fromStdString(PathBar::tabTitle(path.toStdString()))},
            {"path", path},
            {"kind", QStringLiteral("bookmark")},
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
    QVariantList drives = toList(Places::drives());
    if (drives == m_drives) return;
    m_drives = std::move(drives);
    emit drivesChanged();
}
