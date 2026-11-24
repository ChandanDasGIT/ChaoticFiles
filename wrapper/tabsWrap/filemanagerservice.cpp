#include "filemanagerservice.h"

#include "tabscontroller.h"

#include <QDBusConnection>
#include <QUrl>

namespace {

// file:// addresses, and plain paths from apps that send those; anything not on this computer is left out
QStringList localPaths(const QStringList& uris)
{
    QStringList paths;
    for (const QString& uri : uris) {
        const QUrl url = uri.startsWith(QLatin1Char('/')) ? QUrl::fromLocalFile(uri) : QUrl(uri);
        const QString path = url.adjusted(QUrl::StripTrailingSlash).toLocalFile();
        if (!path.isEmpty()) paths << path;
    }
    return paths;
}

} // namespace

FileManagerService::FileManagerService(bool startedToAnswer, QObject* parent)
    : QObject(parent), m_fresh(startedToAnswer)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) return;
    if (!bus.registerObject(QStringLiteral("/org/freedesktop/FileManager1"), this, QDBusConnection::ExportScriptableSlots)) return;
    m_registered = bus.registerService(QStringLiteral("org.freedesktop.FileManager1"));
    if (!m_registered) bus.unregisterObject(QStringLiteral("/org/freedesktop/FileManager1"));
}

bool FileManagerService::takeFirstTab()
{
    const bool fresh = m_fresh;
    m_fresh = false;
    return fresh;
}

void FileManagerService::ShowFolders(const QStringList& uris, const QString&)
{
    TabsController::instance()->showFolders(localPaths(uris), takeFirstTab());
}

void FileManagerService::ShowItems(const QStringList& uris, const QString&)
{
    TabsController::instance()->showItems(localPaths(uris), takeFirstTab());
}

void FileManagerService::ShowItemProperties(const QStringList& uris, const QString&)
{
    const QStringList paths = localPaths(uris);
    TabsController::instance()->showItems(paths, takeFirstTab());
    if (!paths.isEmpty()) emit TabsController::instance()->propertiesRequested(paths);
}
