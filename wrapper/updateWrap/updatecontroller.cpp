#include "wrapper/updateWrap/updatecontroller.h"

#include "hashing/hashing.h"
#include "process/process.h"
#include "update/update.h"
#include "wrapper/operationsWrap/operationscontroller.h"
#include "wrapper/tabsWrap/tabscontroller.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJSEngine>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QUrl>

// The test build replaces these three, to talk to a local server with a test key.
#ifndef UPDATE_API_URL
#define UPDATE_API_URL "https://api.github.com/repos/ChandanDasGIT/ChaoticFiles"
#endif
#ifndef UPDATE_SITE_URL
#define UPDATE_SITE_URL "https://github.com/ChandanDasGIT/ChaoticFiles"
#endif

namespace {

const qint64 kCheckEverySeconds = 24 * 60 * 60;
const int kFirstCheckDelayMs = 3000;
const int kCheckIntervalMs = 60 * 60 * 1000;                     // a file manager can stay open for days
const int kNetworkTimeoutMs = 30000;
const int kStartedDelayMs = 2000;                                // a crash this early still counts as not started
const int kWindowsCheckMs = 500;
const int kWindowsPatience = 10;                                 // checks before saying that a window is taking its time
const qint64 kUnfinishedAfterSeconds = 10 * 60;

QString& startedFile()
{
    static QString path;
    return path;
}
QString& appImagePath()
{
    static QString path;
    return path;
}
QString& appDirPath()
{
    static QString path;
    return path;
}

void leaveMark(const QString& path)
{
    QFile mark(path);
    if (mark.open(QIODevice::WriteOnly)) mark.close();
}

} // namespace

UpdateController* UpdateController::instance()
{
    static UpdateController* controller = new UpdateController(QCoreApplication::instance());
    return controller;
}

UpdateController* UpdateController::create(QQmlEngine*, QJSEngine*)
{
    UpdateController* controller = instance();
    QJSEngine::setObjectOwnership(controller, QJSEngine::CppOwnership);
    return controller;
}

void UpdateController::readEnvironment()
{
    startedFile() = qEnvironmentVariable("CHAOTICFILES_UPDATE_STARTED");
    appImagePath() = qEnvironmentVariable("APPIMAGE");
    appDirPath() = qEnvironmentVariable("APPDIR");
    for (const char* name : {"CHAOTICFILES_UPDATE_STARTED", "APPIMAGE", "APPDIR", "ARGV0", "OWD"}) qunsetenv(name);
}

UpdateController::UpdateController(QObject* parent)
    : QObject(parent)
{
    m_autoCheck = m_settings.value("updates/check", true).toBool();
    m_latest = m_settings.value("updates/latest").toString();

    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/ChaoticFiles"));
    Update::holdInstanceLock((QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
                              + QStringLiteral("/ChaoticFiles/instances.lock")).toStdString());

    // Another program's AppImage can leave its variables behind, so the path only counts if this program runs from it.
    const QString here = QFileInfo(QCoreApplication::applicationDirPath()).canonicalFilePath();
    const QString mounted = QFileInfo(appDirPath()).canonicalFilePath();  // empty if that folder is gone
    const QString tarballCopy = QDir::homePath() + QStringLiteral("/.local/lib/ChaoticFiles");
    if (!appImagePath().isEmpty() && !mounted.isEmpty() && here.startsWith(mounted + QLatin1Char('/'))) {
        const QFileInfo image(appImagePath());
        if (image.isFile() && image.isWritable() && QFileInfo(image.path()).isWritable()) {
            m_kind = Kind::AppImage;
            m_installed = image.absoluteFilePath();
            m_program = m_installed;
        }
    } else if (here == QFileInfo(tarballCopy + QStringLiteral("/usr/bin")).canonicalFilePath()
               && QFileInfo::exists(tarballCopy + QStringLiteral("/AppRun"))
               && QFileInfo(tarballCopy).isWritable() && QFileInfo(QFileInfo(tarballCopy).path()).isWritable()) {
        m_kind = Kind::Tarball;
        m_installed = tarballCopy;
        m_program = tarballCopy + QStringLiteral("/AppRun");
    }

    if (m_kind != Kind::Manual) {
        m_stage = m_installed + QStringLiteral(".update");
        if (QFile::remove(m_stage + QStringLiteral(".failed"))) {
            m_status = QStringLiteral("The update did not start, so the previous version was put back.");
            QTimer::singleShot(0, this, [this] { emit rolledBack(m_status); });
        }
        // left by an unfinished update, unless a script is working in there right now
        const QFileInfo inUse(m_stage + QLatin1Char('/') + QLatin1String(Update::kInstallingFile));
        const bool recent = inUse.exists() && inUse.lastModified().secsTo(QDateTime::currentDateTime()) < kUnfinishedAfterSeconds;
        if (!recent) QDir(m_stage).removeRecursively();
    }

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this] {
        if (m_installing) beginInstall();
    });
    connect(OperationsController::instance(), &OperationsController::operationFinished, this, [this] {
        if (m_waiting && OperationsController::instance()->rowCount() == 0) restartNow();
    });

    m_windowsTimer.setInterval(kWindowsCheckMs);
    connect(&m_windowsTimer, &QTimer::timeout, this, &UpdateController::waitForOtherWindows);
    m_requests.addPath(windowsFolder());
    connect(&m_requests, &QFileSystemWatcher::directoryChanged, this, &UpdateController::closeIfAsked);
    // windows closed for an update, by this version or by the one being replaced
    QTimer::singleShot(kStartedDelayMs, this, &UpdateController::reopenWindows);

    m_dailyCheck.setInterval(kCheckIntervalMs);
    connect(&m_dailyCheck, &QTimer::timeout, this, &UpdateController::considerChecking);
    m_dailyCheck.start();
    QTimer::singleShot(kFirstCheckDelayMs, this, &UpdateController::considerChecking);
}

QString UpdateController::version() const
{
    return QCoreApplication::applicationVersion();
}

bool UpdateController::available() const
{
    return Update::isNewer(m_latest.toStdString(), version().toStdString());
}

void UpdateController::setStatus(const QString& text)
{
    m_status = text;
    emit changed();
}

void UpdateController::fail(const QString& text)
{
    m_busy = false;
    m_ready = false;
    m_download.close();
    if (!m_stage.isEmpty()) QDir(m_stage).removeRecursively();
    setStatus(text);
}

void UpdateController::setAutoCheck(bool on)
{
    if (on == m_autoCheck) return;
    m_autoCheck = on;
    m_settings.setValue("updates/check", on);
    m_settings.sync();
    emit changed();
    considerChecking();
}

void UpdateController::openReleasePage()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral(UPDATE_SITE_URL "/releases/latest")));
}

QString UpdateController::releaseFileUrl(const QString& name) const
{
    return QStringLiteral(UPDATE_SITE_URL "/releases/download/v%1/%2").arg(m_latest, name);
}

QNetworkAccessManager* UpdateController::net()
{
    if (!m_net) m_net = new QNetworkAccessManager(this);
    return m_net;
}

QNetworkReply* UpdateController::get(const QString& url)
{
    QNetworkRequest request {QUrl(url)};
    request.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("ChaoticFiles"));
    request.setTransferTimeout(kNetworkTimeoutMs);
    return net()->get(request);
}

// ------------------------------------------------------------------ is there a new version?

void UpdateController::considerChecking()
{
    if (!m_autoCheck) return;
    const qint64 last = m_settings.value("updates/lastCheck", 0).toLongLong();
    if (QDateTime::currentSecsSinceEpoch() - last >= kCheckEverySeconds) check(false);
}

void UpdateController::checkNow()
{
    if (!m_busy) check(true);
}

void UpdateController::check(bool byHand)
{
    if (m_busy) return;
    m_busy = true;
    if (byHand) setStatus(QStringLiteral("Checking for updates…"));

    QNetworkReply* reply = get(QStringLiteral(UPDATE_API_URL "/releases/latest"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, byHand] {
        reply->deleteLater();
        m_busy = false;
        const QString tag = QJsonDocument::fromJson(reply->readAll()).object().value("tag_name").toString();
        if (reply->error() != QNetworkReply::NoError || !tag.startsWith(QLatin1Char('v'))
            || !Update::isVersion(tag.mid(1).toStdString())) {
            if (byHand) setStatus(QStringLiteral("Could not reach GitHub. Try again later."));
            else emit changed();
            return;                                                // quietly: being offline is normal
        }
        m_latest = tag.mid(1);
        m_settings.setValue("updates/latest", m_latest);
        m_settings.setValue("updates/lastCheck", QDateTime::currentSecsSinceEpoch());
        m_settings.sync();
        if (available()) setStatus(QStringLiteral("Version %1 is available.").arg(m_latest));
        else if (byHand) setStatus(QStringLiteral("You have the latest version."));
        else emit changed();
    });
}

// ------------------------------------------------------------------ getting it

void UpdateController::updateNow()
{
    if (m_busy) return;
    if (m_ready) { askToRestart(); return; }
    if (!available() || !canInstall()) return;

    QDir(m_stage).removeRecursively();
    if (!QDir().mkpath(m_stage)) { fail(QStringLiteral("Could not write the update next to the program.")); return; }
    m_busy = true;
    setStatus(QStringLiteral("Checking the update…"));
    downloadManifest();
}

// The manifest and its signature come first: nothing else is downloaded unless they are right.
void UpdateController::downloadManifest()
{
    QNetworkReply* manifest = get(releaseFileUrl(QStringLiteral("update-manifest.txt")));
    connect(manifest, &QNetworkReply::finished, this, [this, manifest] {
        manifest->deleteLater();
        if (manifest->error() != QNetworkReply::NoError) {
            fail(QStringLiteral("This version has no signed update yet. Try again later."));
            return;
        }
        m_manifest = manifest->readAll();
        QNetworkReply* signature = get(releaseFileUrl(QStringLiteral("update-manifest.txt.sig")));
        connect(signature, &QNetworkReply::finished, this, [this, signature] {
            signature->deleteLater();
            if (signature->error() != QNetworkReply::NoError) {
                fail(QStringLiteral("This version has no signed update yet. Try again later."));
                return;
            }
            if (!Update::signatureIsValid(m_manifest.toStdString(), signature->readAll().toStdString(), UPDATE_PUBLIC_KEY)) {
                fail(QStringLiteral("The update's signature is wrong, so it was not installed."));
                return;
            }
            const auto manifest = Update::parseManifest(m_manifest.toStdString());
            if (!manifest || QString::fromStdString(manifest->version) != m_latest
                || !Update::isNewer(manifest->version, version().toStdString())) {
                fail(QStringLiteral("The update's details are wrong, so it was not installed."));
                return;
            }
            const bool tarball = m_kind == Kind::Tarball;
            m_fileName = QString::fromStdString(tarball ? manifest->tarball : manifest->appImage);
            m_fileHash = QString::fromStdString(tarball ? manifest->tarballHash : manifest->appImageHash);
            downloadFile();
        });
    });
}

void UpdateController::downloadFile()
{
    m_download.close();
    m_download.setFileName(m_stage + QLatin1Char('/') + m_fileName);
    if (!m_download.open(QIODevice::WriteOnly)) { fail(QStringLiteral("Could not write the update next to the program.")); return; }

    QNetworkReply* reply = get(releaseFileUrl(m_fileName));
    const auto save = [this, reply] {
        const QByteArray bytes = reply->readAll();
        if (m_download.write(bytes) != bytes.size()) reply->abort();
    };
    connect(reply, &QNetworkReply::readyRead, this, save);
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 done, qint64 total) {
        if (total > 0) setStatus(QStringLiteral("Downloading… %1%").arg(done * 100 / total));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, save] {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) save();
        const bool complete = reply->error() == QNetworkReply::NoError && m_download.flush();
        m_download.close();
        if (!complete) { fail(QStringLiteral("The download failed. Try again later.")); return; }
        checkDownload();
    });
}

// Hashing and unpacking take seconds, so they run off the UI thread.
void UpdateController::checkDownload()
{
    setStatus(QStringLiteral("Checking the download…"));
    const std::string file = m_download.fileName().toStdString();
    const std::string hash = m_fileHash.toStdString();
    const std::string folder = (m_stage + QStringLiteral("/unpacked")).toStdString();
    const bool tarball = m_kind == Kind::Tarball;

    m_job.start([this, file, hash, folder, tarball](const BackgroundJob::Handle& handle) {
        enum class Result { Good, WrongHash, NotUnpacked };
        const auto finish = [this, handle](Result result) {
            handle.post([this, result] {
                QMetaObject::invokeMethod(this, [this, result] {
                    if (result == Result::WrongHash) { fail(QStringLiteral("The download does not match its signature, so it was not installed.")); return; }
                    if (result == Result::NotUnpacked) { fail(QStringLiteral("Could not unpack the update.")); return; }
                    if (m_kind == Kind::AppImage) {
                        m_download.setPermissions(m_download.permissions() | QFile::ExeOwner | QFile::ExeGroup | QFile::ExeOther);
                        readyForRestart(m_download.fileName());
                        return;
                    }
                    const QString unpacked = m_stage + QStringLiteral("/unpacked");
                    const QStringList top = QDir(unpacked).entryList(QDir::Dirs | QDir::NoDotAndDotDot);
                    const QString copy = top.size() == 1 ? unpacked + QLatin1Char('/') + top[0] + QStringLiteral("/app") : QString();
                    if (copy.isEmpty() || !QFileInfo(copy + QStringLiteral("/AppRun")).isExecutable()) { fail(QStringLiteral("Could not unpack the update.")); return; }
                    QFile::remove(m_download.fileName());          // the tarball is not needed any more
                    readyForRestart(copy);
                }, Qt::QueuedConnection);
            });
        };

        std::string found;
        for (const HashResult& result : Hashing::hashFile(file, {}, [&handle] { return handle.cancelled(); }))
            if (result.name == "SHA-256") found = result.value;
        if (found != hash) { finish(Result::WrongHash); return; }
        if (tarball) {
            QDir().mkpath(QString::fromStdString(folder));
            const Process::Result unpacked = Process::run({"tar", "-xJf", file, "-C", folder}, [&handle] { return handle.cancelled(); });
            if (unpacked.exitCode != 0) { finish(Result::NotUnpacked); return; }
        }
        finish(Result::Good);
    });
}

void UpdateController::readyForRestart(const QString& newCopy)
{
    m_newCopy = newCopy;
    m_busy = false;
    m_ready = true;
    setStatus(QStringLiteral("Version %1 is ready. Restart to finish.").arg(m_latest));
    askToRestart();
}

// ------------------------------------------------------------------ putting it in place

// Nothing here changes what quitting does. Only restartNow does, so Cancel leaves every later quit a normal one.
void UpdateController::askToRestart()
{
    emit restartWanted(OperationsController::instance()->rowCount());
}

bool UpdateController::otherWindowsOpen() const
{
    return Update::otherInstancesRunning();
}

QString UpdateController::windowsFolder() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/ChaoticFiles");
}

// Runs in the window that restarts, until the others are gone. Asking again keeps the request alive, and lets a
// window that was busy, or has just been opened, see it.
void UpdateController::waitForOtherWindows()
{
    if (Update::otherInstancesRunning()) {
        Update::askWindowsToClose(windowsFolder().toStdString(), QCoreApplication::applicationPid());
        if (++m_windowsWaits == kWindowsPatience)
            setStatus(QStringLiteral("Waiting for another ChaoticFiles window. It closes when its file operations finish, or you can close it."));
        return;
    }
    m_windowsTimer.stop();
    m_closingWindows = false;
    Update::stopAskingWindowsToClose(windowsFolder().toStdString(), QCoreApplication::applicationPid());
    if (OperationsController::instance()->rowCount() > 0) restartWhenFinished();
    else restartNow();
}

// Runs in every other window. This window's own file operations finish first, as they do for a restart.
void UpdateController::closeIfAsked()
{
    const int asking = Update::closeRequester(windowsFolder().toStdString());
    if (asking == 0 || asking == QCoreApplication::applicationPid()) return;
    if (m_installing || OperationsController::instance()->rowCount() > 0) return;

    m_windowsTimer.stop();                                           // two windows asked at once, and the other one won
    std::vector<std::string> arguments;
    for (const QString& folder : TabsController::instance()->folders()) arguments.push_back(folder.toStdString());
    arguments.push_back("--tab=" + std::to_string(TabsController::instance()->currentIndex()));
    Update::rememberWindow(windowsFolder().toStdString(), QCoreApplication::applicationPid(), arguments);
    QCoreApplication::quit();
}

void UpdateController::reopenWindows()
{
    const std::string folder = windowsFolder().toStdString();
    if (Update::closeRequester(folder) != 0) return;                 // those windows wait for the update to finish
    const std::string program = m_program.isEmpty() ? QCoreApplication::applicationFilePath().toStdString() : m_program.toStdString();
    for (std::vector<std::string> arguments : Update::takeRememberedWindows(folder)) {
        arguments.insert(arguments.begin(), program);
        Process::startDetached(arguments, QDir::homePath().toStdString());
    }
}

void UpdateController::restartWhenFinished()
{
    m_waiting = true;
    setStatus(QStringLiteral("Restarting when the file operations finish."));
    if (OperationsController::instance()->rowCount() == 0) restartNow();
}

void UpdateController::cancelRestart()
{
    m_waiting = false;
    if (m_closingWindows) {
        m_windowsTimer.stop();
        m_closingWindows = false;
        Update::stopAskingWindowsToClose(windowsFolder().toStdString(), QCoreApplication::applicationPid());
        reopenWindows();                                             // the ones that closed already
    }
    setStatus(QStringLiteral("Version %1 is ready. Restart to finish.").arg(m_latest));
}

void UpdateController::restartNow()
{
    if (!m_ready) return;
    m_waiting = false;
    if (Update::otherInstancesRunning()) {
        m_closingWindows = true;
        m_windowsWaits = 0;
        setStatus(QStringLiteral("Closing the other ChaoticFiles windows…"));
        Update::askWindowsToClose(windowsFolder().toStdString(), QCoreApplication::applicationPid());
        m_windowsTimer.start();
        return;
    }
    m_installing = true;
    QCoreApplication::quit();
}

void UpdateController::beginInstall()
{
    const QString scriptPath = m_stage + QStringLiteral("/install.sh");
    QFile script(scriptPath);
    if (!script.open(QIODevice::WriteOnly)) return;
    script.write(Update::installScript());
    script.close();
    leaveMark(m_stage + QLatin1Char('/') + QLatin1String(Update::kInstallingFile));

    const std::string installed = m_installed.toStdString();
    const std::string target = m_kind == Kind::AppImage ? Update::appImageTarget(installed, version().toStdString(), m_fileName.toStdString()) : installed;
    const std::string program = m_kind == Kind::AppImage ? target : m_program.toStdString();
    std::vector<std::string> command = {"/bin/sh", scriptPath.toStdString(), m_stage.toStdString(), m_newCopy.toStdString(),
                                        installed, target, program, m_program.toStdString()};
    for (const QString& folder : TabsController::instance()->folders()) command.push_back(folder.toStdString());
    command.push_back("--tab=" + std::to_string(TabsController::instance()->currentIndex()));
    if (!Process::startDetached(command, QDir::homePath().toStdString())) m_installing = false;
}

void UpdateController::reportStarted()
{
    if (startedFile().isEmpty()) return;
    const QString path = startedFile();
    startedFile().clear();
    leaveMark(path);
}

void UpdateController::reportExited()
{
    if (m_installing) leaveMark(m_stage + QLatin1Char('/') + QLatin1String(Update::kOldExitedFile));
}
