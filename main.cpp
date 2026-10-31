#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QStandardPaths>

#include "wrapper/appsWrap/appiconprovider.h"
#include "wrapper/tabsWrap/tabscontroller.h"
#include "wrapper/thumbnailsWrap/thumbnailprovider.h"

// AppRun points the loader at the bundled libraries. The loader keeps that value from here on, so the user's
// own one goes back: every app opened from this one would otherwise load the bundled libraries and may crash.
static void restoreLibraryPath()
{
    if (!qEnvironmentVariableIsSet("CHAOTICFILES_USER_LD_LIBRARY_PATH")) return;
    const QByteArray original = qgetenv("CHAOTICFILES_USER_LD_LIBRARY_PATH");
    if (original.isEmpty()) qunsetenv("LD_LIBRARY_PATH");
    else qputenv("LD_LIBRARY_PATH", original);
    qunsetenv("CHAOTICFILES_USER_LD_LIBRARY_PATH");
}

int main(int argc, char *argv[])
{
    restoreLibraryPath();

    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("ChaoticFiles");
    QCoreApplication::setApplicationName("ChaoticFiles");
    // Qt registers this name with the desktop portal, which refuses a name with no installed .desktop file
    if (!QStandardPaths::locate(QStandardPaths::ApplicationsLocation, "ChaoticFiles.desktop").isEmpty())
        QGuiApplication::setDesktopFileName("ChaoticFiles");

    // folders named on the command line open in tabs: appChaoticFiles ~/Music "My Folder"
    QStringList folders;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args.at(i).startsWith(QLatin1Char('-'))) continue;                   // an option, not a folder
        folders << QFileInfo(args.at(i)).absoluteFilePath();                     // relative names count from the current folder
    }
    TabsController::instance()->start(folders);

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.addImageProvider("thumb", new ThumbnailProvider);
    engine.addImageProvider("appicon", new AppIconProvider);
    engine.loadFromModule("ChaoticFiles", "Main");

    return QGuiApplication::exec();
}
