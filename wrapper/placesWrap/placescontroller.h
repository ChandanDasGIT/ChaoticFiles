#pragma once
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// The sidebar's lists (PlacesController in QML). Drives are checked every few seconds, so one that is
// plugged in or removed shows up by itself.
class PlacesController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList places READ places CONSTANT)
    Q_PROPERTY(QVariantList drives READ drives NOTIFY drivesChanged)

public:
    explicit PlacesController(QObject* parent = nullptr);

    QVariantList places() const { return m_places; }
    QVariantList drives() const { return m_drives; }

signals:
    void drivesChanged();

private:
    void updateDrives();

    QVariantList m_places;
    QVariantList m_drives;
    QTimer m_timer;
};
