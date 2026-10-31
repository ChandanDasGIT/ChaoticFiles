#pragma once
#include "diskusage/diskusage.h"
#include "tasks/backgroundjob.h"

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QQmlEngine;

// The Disk Usage window (DiskUsageController in QML): what takes up the space in one folder. The window fills in
// from a background thread, biggest first, and closing it stops the scan.
class DiskUsageController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool isOpen READ isOpen NOTIFY openChanged)
    Q_PROPERTY(QString folder READ folder NOTIFY reportChanged)
    Q_PROPERTY(QVariantList items READ items NOTIFY reportChanged)     // [{name, path, sizeText, share, isFolder}]
    Q_PROPERTY(QString summary READ summary NOTIFY reportChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY reportChanged)
    Q_PROPERTY(bool canGoUp READ canGoUp NOTIFY reportChanged)

public:
    static DiskUsageController* instance();
    static DiskUsageController* create(QQmlEngine*, QJSEngine*);

    bool isOpen() const { return m_open; }
    QString folder() const { return m_folder; }
    QVariantList items() const { return m_items; }
    QString summary() const { return m_summary; }
    bool scanning() const { return m_scanning; }
    bool canGoUp() const;

    Q_INVOKABLE void show(const QString& folder);
    Q_INVOKABLE void openFolder(const QString& folder);
    Q_INVOKABLE void goUp();
    Q_INVOKABLE void close();

signals:
    void openChanged();
    void reportChanged();

private:
    explicit DiskUsageController(QObject* parent);

    void apply(const UsageReport& report);

    BackgroundJob m_job;
    quint64 m_generation = 0;
    bool m_open = false;
    bool m_scanning = false;
    QString m_folder;
    QString m_summary;
    QVariantList m_items;
};
