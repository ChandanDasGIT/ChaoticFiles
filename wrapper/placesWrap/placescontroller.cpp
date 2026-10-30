#include "placescontroller.h"

#include "places/places.h"

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
    connect(&m_timer, &QTimer::timeout, this, &PlacesController::updateDrives);
    m_timer.start(kDriveCheckMs);
}

void PlacesController::updateDrives()
{
    QVariantList drives = toList(Places::drives());
    if (drives == m_drives) return;
    m_drives = std::move(drives);
    emit drivesChanged();
}
