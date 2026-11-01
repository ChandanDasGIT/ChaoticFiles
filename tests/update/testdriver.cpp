#include "tests/update/testdriver.h"

#include "wrapper/operationsWrap/operationscontroller.h"
#include "wrapper/settingsWrap/settingscontroller.h"
#include "wrapper/diskUsageWrap/diskusagecontroller.h"
#include "wrapper/folderWrap/foldermodel.h"
#include "wrapper/folderWrap/foldersession.h"
#include "wrapper/tabsWrap/tabscontroller.h"
#include "wrapper/updateWrap/updatecontroller.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QTest>

namespace {

QQuickWindow* mainWindow(QQmlApplicationEngine* engine)
{
    return engine->rootObjects().isEmpty() ? nullptr : qobject_cast<QQuickWindow*>(engine->rootObjects().first());
}

void collect(QQuickItem* item, QList<QQuickItem*>& items)
{
    items << item;
    for (QQuickItem* child : item->childItems()) collect(child, items);
}

bool reallyVisible(QQuickItem* item)
{
    return item->isVisible() && item->width() > 0 && item->height() > 0;
}

// by objectName, or by the words on a button
QQuickItem* find(QQuickWindow* window, const QString& wanted)
{
    QList<QQuickItem*> items;
    collect(window->contentItem(), items);
    QQuickItem* byText = nullptr;
    for (QQuickItem* item : items) {
        if (!reallyVisible(item)) continue;
        if (item->objectName() == wanted) return item;
        if (item->property("text").toString() == wanted && item->property("primary").isValid()) byText = item;
    }
    return byText;
}

QStringList rowNames()
{
    QStringList names;
    QAbstractItemModel* model = TabsController::instance()->currentSession()->model();
    for (int row = 0; row < model->rowCount(); ++row) names << model->index(row, 0).data(FolderModel::NameRole).toString();
    return names;
}

QStringList usageNames()
{
    QStringList names;
    for (const QVariant& item : DiskUsageController::instance()->items()) names << item.toMap().value("name").toString();
    return names;
}

} // namespace

TestDriver::TestDriver(QQmlApplicationEngine* engine)
    : QObject(engine), m_engine(engine)
{
    // a restarted app keeps the same files, and must not run the old commands again
    m_position = QFile(qEnvironmentVariable("CF_TEST_COMMANDS")).size();
    m_timer.setInterval(50);
    connect(&m_timer, &QTimer::timeout, this, &TestDriver::poll);
    m_timer.start();
    run("ready");
}

void TestDriver::poll()
{
    QFile commands(qEnvironmentVariable("CF_TEST_COMMANDS"));
    if (!commands.open(QIODevice::ReadOnly) || !commands.seek(m_position)) return;
    const QByteArray fresh = commands.readAll();
    const int end = fresh.lastIndexOf('\n');
    if (end < 0) return;
    m_position += end + 1;
    for (const QByteArray& line : fresh.left(end).split('\n')) run(line);
}

void TestDriver::run(const QByteArray& line)
{
    const int space = line.indexOf(' ');
    const QByteArray command = space < 0 ? line : line.left(space);
    const QString argument = space < 0 ? QString() : QString::fromUtf8(line.mid(space + 1));
    QQuickWindow* window = mainWindow(m_engine);
    bool ok = window != nullptr;

    if (command == "click" && window) {
        QQuickItem* item = find(window, argument);
        ok = item != nullptr;
        if (item) {
            const QPointF centre = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
            ok = QRectF(0, 0, window->width(), window->height()).contains(centre);     // a user couldn't click it either
            if (ok) QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, centre.toPoint());
        }
    } else if (command == "visible" && window) {
        ok = find(window, argument) != nullptr;
    } else if (command == "resize" && window) {
        const QStringList size = argument.split(QLatin1Char('x'));
        window->resize(size[0].toInt(), size[1].toInt());
    } else if (command == "shot" && window) {
        window->grabWindow().save(argument);
    } else if (command == "copy") {                          // copy <file>|<folder>
        const QStringList paths = argument.split(QLatin1Char('|'));
        OperationsController::instance()->copy({paths[0]}, paths[1]);
    } else if (command == "pause") {                         // pauses the first operation, or resumes it
        OperationsController* operations = OperationsController::instance();
        ok = operations->rowCount() > 0;
        if (ok) operations->pause(operations->data(operations->index(0), OperationsController::IdRole).toInt());
    } else if (command == "key" && window) {                 // a real key press
        QTest::keyClick(window, argument.at(0).toLatin1());
    } else if (command == "clickrow" && window) {            // the mouse on a file: click <x>,<y> in the window
        const QStringList at = argument.split(QLatin1Char(','));
        QTest::mouseClick(window, Qt::LeftButton, Qt::NoModifier, QPoint(at[0].toInt(), at[1].toInt()));
    } else if (command == "drag" && window) {                // the mouse held down: drag <x>,<y>|<x>,<y>
        const QStringList ends = argument.split(QLatin1Char('|'));
        const auto point = [](const QString& text) { const QStringList at = text.split(QLatin1Char(',')); return QPoint(at[0].toInt(), at[1].toInt()); };
        const QPoint from = point(ends[0]), to = point(ends[1]);
        QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, from);
        constexpr int kSteps = 12;
        for (int step = 1; step <= kSteps; ++step) QTest::mouseMove(window, from + (to - from) * step / kSteps, 10);
        if (ends.size() > 2) window->grabWindow().save(ends[2]);     // drag ...|<picture of the drag before letting go>
        QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, to);
    } else if (command == "dropurl" && window) {             // a drop from another app: dropurl <x>,<y>|<path>
        const QStringList parts = argument.split(QLatin1Char('|'));
        const QStringList at = parts[0].split(QLatin1Char(','));
        const QPointF point(at[0].toInt(), at[1].toInt());
        QMimeData mime;
        mime.setUrls({QUrl::fromLocalFile(parts[1])});
        QDragEnterEvent enter(point.toPoint(), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(window, &enter);
        QDragMoveEvent move(point.toPoint(), Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(window, &move);
        QDropEvent drop(point, Qt::CopyAction, &mime, Qt::LeftButton, Qt::NoModifier);
        QCoreApplication::sendEvent(window, &drop);
    } else if (command == "searchinside") {
        TabsController::instance()->currentSession()->search(argument, true);
    } else if (command == "diskusage") {
        DiskUsageController::instance()->show(argument);
    } else if (command == "openwith" && window) {
        QObject* dialog = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("openWithDialog"));
        ok = dialog && QMetaObject::invokeMethod(dialog, "ask", Q_ARG(QVariant, QVariant(QStringList {argument})));
    } else if (command == "scrolltop" && window) {           // the settings, back at their first line
        QObject* settings = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("settingsDialog"));
        ok = settings && QMetaObject::invokeMethod(settings, "scrollToTop");
    } else if (command == "grid") {
        SettingsController::instance()->setViewMode(1);
    } else if (command == "about" && window) {
        QObject* settings = window->findChild<QObject*>(QStringLiteral("settingsDialog"));
        if (!settings) settings = m_engine->rootObjects().first()->findChild<QObject*>(QStringLiteral("settingsDialog"));
        ok = settings && QMetaObject::invokeMethod(settings, "openAbout");
    } else if (command == "quit") {
        QCoreApplication::quit();
    }

    UpdateController* update = UpdateController::instance();
    const QStringList folders = TabsController::instance()->folders();
    const QJsonObject answer {
        {"command", QString::fromUtf8(line)},
        {"ok", ok},
        {"pid", QCoreApplication::applicationPid()},
        {"version", update->version()},
        {"available", update->available()},
        {"status", update->status()},
        {"busy", update->busy()},
        {"canInstall", update->canInstall()},
        {"ready", update->readyToRestart()},
        {"waiting", update->waitingForOperations()},
        {"autoCheck", update->autoCheck()},
        {"operations", OperationsController::instance()->rowCount()},
        {"tabs", QJsonArray::fromStringList(folders)},
        {"rows", QJsonArray::fromStringList(rowNames())},
        {"loading", TabsController::instance()->currentSession()->loading()},
        {"usage", QJsonArray::fromStringList(usageNames())},
        {"usageScanning", DiskUsageController::instance()->scanning()},
        {"selected", QJsonArray::fromStringList(TabsController::instance()->currentSession()->selectedPaths())},
        {"focus", window && window->activeFocusItem() ? QString::fromLatin1(window->activeFocusItem()->metaObject()->className()) : QString()},
        {"currentTab", TabsController::instance()->currentIndex()},
    };
    QFile answers(qEnvironmentVariable("CF_TEST_ANSWERS"));
    if (answers.open(QIODevice::Append)) answers.write(QJsonDocument(answer).toJson(QJsonDocument::Compact) + '\n');
}
