#pragma once
#include "tasks/backgroundjob.h"

#include <QObject>
#include <QSettings>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// The sidebar's lists (PlacesController in QML). Drives and network folders are checked every few seconds on a
// background thread, so one that is plugged in or removed shows up by itself.
class PlacesController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantList places READ places CONSTANT)
    Q_PROPERTY(QVariantList bookmarks READ bookmarks NOTIFY bookmarksChanged)
    Q_PROPERTY(QVariantList drives READ drives NOTIFY drivesChanged)      // [{name, path, kind, device, mounted}]
    Q_PROPERTY(QVariantList shares READ shares NOTIFY drivesChanged)      // [{name, path, kind, uri}]

public:
    explicit PlacesController(QObject* parent = nullptr);

    QVariantList places() const { return m_places; }
    QVariantList drives() const { return m_drives; }
    QVariantList shares() const { return m_shares; }
    QVariantList bookmarks() const;

    Q_INVOKABLE void mount(const QString& device);
    Q_INVOKABLE void unmount(const QString& device);
    Q_INVOKABLE void connectShare(const QString& address, const QString& user, const QString& password);
    Q_INVOKABLE void disconnectShare(const QString& uri);

    Q_INVOKABLE void addBookmark(const QString& path);
    Q_INVOKABLE void removeBookmark(const QString& path);
    Q_INVOKABLE bool isBookmarked(const QString& path) const;

signals:
    void drivesChanged();
    void bookmarksChanged();
    void mounted(const QString& path);                // a drive or share is ready: go there
    void connectFinished(const QString& failure);     // empty when it worked
    void problem(const QString& title, const QString& text);

private:
    void updateDrives();

    QVariantList m_places;
    QVariantList m_drives;
    QVariantList m_shares;
    BackgroundJob m_scan;
    BackgroundJob m_action;
    QSettings m_settings;
    QTimer m_timer;
};
