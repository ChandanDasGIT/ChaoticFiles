#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include "wrapper/tabsWrap/tabscontroller.h"
#include "wrapper/thumbnailsWrap/thumbnailprovider.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName("ChaoticFiles");
    QCoreApplication::setApplicationName("ChaoticFiles");
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
    engine.loadFromModule("ChaoticFiles", "Main");

    return QGuiApplication::exec();
}
