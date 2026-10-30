#pragma once
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// Open With and the terminal (AppsController in QML).
class AppsController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit AppsController(QObject* parent = nullptr);

    Q_INVOKABLE QVariantList appsFor(const QString& path) const;     // [{id, name}], the default app first
    Q_INVOKABLE QVariantList allApps() const;
    Q_INVOKABLE bool open(const QString& appId, const QStringList& paths) const;
    Q_INVOKABLE bool openTerminal(const QString& folder) const;
};
