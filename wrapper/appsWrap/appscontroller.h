#pragma once
#include "tasks/backgroundjob.h"

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// Open With, default apps and the terminal (AppsController in QML).
class AppsController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    // [{name, mimeTypes, types: [{label, mimeTypes, appId, appName, appIcon}]}]; loadFileTypes() fills it
    Q_PROPERTY(QVariantList fileTypes READ fileTypes NOTIFY fileTypesChanged)

public:
    explicit AppsController(QObject* parent = nullptr);

    QVariantList fileTypes() const { return m_fileTypes; }

    Q_INVOKABLE QVariantList appsFor(const QString& path) const;     // [{id, name, icon}], the default app first
    Q_INVOKABLE QVariantList appsForTypes(const QStringList& mimeTypes) const;
    Q_INVOKABLE QVariantList allApps() const;
    Q_INVOKABLE QVariantMap typeOf(const QString& path) const;       // {label, mimeType, defaultApp}, as opening the file sees it
    Q_INVOKABLE QString defaultApp(const QString& mimeType) const;
    Q_INVOKABLE bool open(const QString& appId, const QStringList& paths) const;
    Q_INVOKABLE bool openTerminal(const QString& folder) const;

    Q_INVOKABLE void loadFileTypes();                                // asks in the background
    // onlySupported leaves out the types the app does not say it opens, for setting a whole category
    Q_INVOKABLE void setDefault(const QString& appId, const QStringList& mimeTypes, bool onlySupported);

signals:
    void fileTypesChanged();
    void message(const QString& text);

private:
    QVariantList m_fileTypes;
    BackgroundJob m_load;
};
