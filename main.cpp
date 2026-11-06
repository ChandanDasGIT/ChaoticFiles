#include <QApplication>
#include <QFileInfo>
#include <QQmlApplicationEngine>
#include <QStandardPaths>
#include <QTimer>

#include "wrapper/appsWrap/appiconprovider.h"
#include "wrapper/tabsWrap/tabscontroller.h"
#include "wrapper/thumbnailsWrap/thumbnailprovider.h"
#include "wrapper/updateWrap/updatecontroller.h"

#ifdef CHAOTICFILES_TEST_DRIVER
#include "tests/update/testdriver.h"
#endif

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
    UpdateController::readEnvironment();

    QApplication app(argc, argv);           // not QGuiApplication: the menus are QMenus (Qt Widgets)
    QCoreApplication::setOrganizationName("ChaoticFiles");
    QCoreApplication::setApplicationName("ChaoticFiles");
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION));
    // Qt registers this name with the desktop portal, which refuses a name with no installed .desktop file
    if (!QStandardPaths::locate(QStandardPaths::ApplicationsLocation, "ChaoticFiles.desktop").isEmpty())
        QGuiApplication::setDesktopFileName("ChaoticFiles");

    // folders named on the command line open in tabs: appChaoticFiles ~/Music "My Folder"
    QStringList folders;
    int currentTab = 0;
    const QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i) {
        if (args.at(i).startsWith(QStringLiteral("--tab="))) currentTab = args.at(i).mid(6).toInt();   // an update reopening the tabs
        if (args.at(i).startsWith(QLatin1Char('-'))) continue;                   // an option, not a folder
        folders << QFileInfo(args.at(i)).absoluteFilePath();                     // relative names count from the current folder
    }
    TabsController::instance()->start(folders, currentTab);

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

    // An update only counts once the app has really been running. Closing it straight away counts too.
    if (!engine.rootObjects().isEmpty()) {
        QTimer::singleShot(2000, &app, &UpdateController::reportStarted);
        QObject::connect(&app, &QCoreApplication::aboutToQuit, &UpdateController::reportStarted);
    }
#ifdef CHAOTICFILES_TEST_DRIVER
    TestDriver driver(&engine);
#endif

    const int code = QApplication::exec();
    UpdateController::instance()->reportExited();
    return code;
}
