#pragma once
#include <QObject>
#include <QStringList>

// org.freedesktop.FileManager1 on the session bus: what browsers call for "Show in folder", and other apps for
// "open the folder". Only one program can hold the name; if another file manager has it, this stays quiet.
class FileManagerService : public QObject {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.FileManager1")

public:
    // `startedToAnswer`: D-Bus started this window for a call, so the first call takes over its first tab
    explicit FileManagerService(bool startedToAnswer, QObject* parent = nullptr);
    bool registered() const { return m_registered; }

public slots:
    Q_SCRIPTABLE void ShowFolders(const QStringList& uris, const QString& startupId);
    Q_SCRIPTABLE void ShowItems(const QStringList& uris, const QString& startupId);
    Q_SCRIPTABLE void ShowItemProperties(const QStringList& uris, const QString& startupId);

private:
    bool takeFirstTab();

    bool m_fresh;
    bool m_registered = false;
};
