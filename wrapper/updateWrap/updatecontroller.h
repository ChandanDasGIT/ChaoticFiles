#pragma once
#include "tasks/backgroundjob.h"
#include "update/update.h"

#include <QFile>
#include <QFileSystemWatcher>
#include <QObject>
#include <QSettings>
#include <QString>
#include <QTimer>
#include <QtQml/qqmlregistration.h>

class QJSEngine;
class QNetworkAccessManager;
class QNetworkReply;
class QQmlEngine;

// New versions (UpdateController in QML). Once a day, if the switch is on, it asks GitHub for the latest release:
// a newer one shows the green dot in the status bar. The update itself only starts from Update now in the settings.
//
// How an update stays safe: every release has update-manifest.txt (version, file names and their SHA-256) and
// update-manifest.txt.sig, signed in the release workflow with the private key. The public key is built into
// the app, and nothing is installed unless the signature, the version and the file's hash are all right.
//
// A tarball install and an AppImage can update themselves. Anything else, like an install that needs root, gets
// the dot and a link to the release page.
//
// When the new files are ready the app asks before it restarts. Running file operations stand in for unsaved work:
// it waits for them. Other windows of the app close too, each after its own file operations, and leave their tabs
// behind. A small script then swaps the copies (see Update::installScript), and every window opens again.
class UpdateController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(bool available READ available NOTIFY changed)           // a newer version exists: the green dot
    Q_PROPERTY(QString latestVersion READ latestVersion NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)              // the text in the About section
    Q_PROPERTY(bool busy READ busy NOTIFY changed)                     // checking or downloading
    Q_PROPERTY(bool canInstall READ canInstall CONSTANT)               // false: only the release page can help
    Q_PROPERTY(bool readyToRestart READ readyToRestart NOTIFY changed) // downloaded and checked
    Q_PROPERTY(bool waitingForOperations READ waitingForOperations NOTIFY changed)
    Q_PROPERTY(bool autoCheck READ autoCheck WRITE setAutoCheck NOTIFY changed)

public:
    static UpdateController* instance();
    static UpdateController* create(QQmlEngine*, QJSEngine*);

    QString version() const;
    bool available() const;
    QString latestVersion() const { return m_latest; }
    QString status() const { return m_status; }
    bool busy() const { return m_busy; }
    bool canInstall() const { return m_kind != Kind::Manual; }
    bool readyToRestart() const { return m_ready; }
    bool waitingForOperations() const { return m_waiting || m_closingWindows; }
    bool autoCheck() const { return m_autoCheck; }
    void setAutoCheck(bool on);

    Q_INVOKABLE void checkNow();
    Q_INVOKABLE void updateNow();
    Q_INVOKABLE void restartNow();                 // the answer to restartWanted
    Q_INVOKABLE bool otherWindowsOpen() const;
    Q_INVOKABLE void restartWhenFinished();
    Q_INVOKABLE void cancelRestart();
    Q_INVOKABLE void openReleasePage();

    // Takes what the launcher left in the environment, so that the apps opened from this one don't inherit it.
    static void readEnvironment();
    // The new version's window exists: the install script then knows the update worked.
    static void reportStarted();
    // The last thing the program does. The install script waits for it.
    void reportExited();

signals:
    void changed();
    void restartWanted(int operationsRunning);     // asks before anything closes
    void rolledBack(const QString& text);

private:
    enum class Kind { Tarball, AppImage, Manual };

    explicit UpdateController(QObject* parent);

    QNetworkAccessManager* net();
    QNetworkReply* get(const QString& url);
    QString releaseFileUrl(const QString& name) const;
    void considerChecking();
    void check(bool byHand);
    void downloadManifest();
    void downloadFile();
    void checkDownload();
    void readyForRestart(const QString& newCopy);
    void askToRestart();
    void waitForOtherWindows();
    void closeIfAsked();
    void reopenWindows();
    QString windowsFolder() const;
    void beginInstall();
    void fail(const QString& text);
    void setStatus(const QString& text);

    QSettings m_settings;
    QNetworkAccessManager* m_net = nullptr;        // made at the first request: the switch off means no network at all
    QTimer m_dailyCheck;
    BackgroundJob m_job;
    QTimer m_windowsTimer;                         // renews the request while the other windows close
    QFileSystemWatcher m_requests;                 // sees another window's request
    int m_windowsWaits = 0;

    Kind m_kind = Kind::Manual;
    QString m_latest;                              // the newest version known, without the v
    QString m_status;
    bool m_busy = false;
    bool m_ready = false;
    bool m_waiting = false;                        // restarts when the file operations finish
    bool m_closingWindows = false;                 // restarts when the other windows have closed
    bool m_autoCheck = true;
    bool m_installing = false;                     // only set as the app quits for the update

    QString m_installed;                           // what the update replaces: the program's folder, or the AppImage
    QString m_stage;                               // the update's own folder next to it, so the swap is a plain rename
    QString m_newCopy;                             // the new folder or AppImage, ready to take its place
    QString m_program;                             // what to start afterwards
    QByteArray m_manifest;
    QString m_fileName;                            // from the signed manifest
    QString m_fileHash;
    QFile m_download;
};
