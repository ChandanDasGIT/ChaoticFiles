#include "clipboardcontroller.h"

#include "operationscontroller.h"
#include "tabscontroller.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>
#include <QUrl>

namespace {
const auto kGnomeFormat = QStringLiteral("x-special/gnome-copied-files");
}

ClipboardController::ClipboardController(QObject* parent)
    : QObject(parent)
{
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, [this] {
        const QMimeData* data = QGuiApplication::clipboard()->mimeData();
        const bool stillCut = data && data->data(kGnomeFormat).startsWith("cut\n");
        if (!stillCut) TabsController::instance()->setCutPaths({});
        emit changed();
    });
}

bool ClipboardController::canPaste() const
{
    const QMimeData* data = QGuiApplication::clipboard()->mimeData();
    return data && data->hasUrls();
}

void ClipboardController::put(const QStringList& paths, bool cutting)
{
    QList<QUrl> urls;
    QByteArray special = cutting ? "cut" : "copy";
    QStringList lines;
    for (const QString& path : paths) {
        const QUrl url = QUrl::fromLocalFile(path);
        urls << url;
        special += '\n' + url.toEncoded();
        lines << path;
    }
    auto* data = new QMimeData;
    data->setUrls(urls);
    data->setText(lines.join(QLatin1Char('\n')));
    data->setData(kGnomeFormat, special);
    QGuiApplication::clipboard()->setMimeData(data);
    TabsController::instance()->setCutPaths(cutting ? paths : QStringList());
}

void ClipboardController::copy(const QStringList& paths)
{
    put(paths, false);
}

void ClipboardController::cut(const QStringList& paths)
{
    put(paths, true);
}

void ClipboardController::paste(const QString& destination)
{
    const QMimeData* data = QGuiApplication::clipboard()->mimeData();
    if (!data || !data->hasUrls()) return;

    QStringList paths;
    for (const QUrl& url : data->urls())
        if (url.isLocalFile()) paths << url.toLocalFile();

    if (data->data(kGnomeFormat).startsWith("cut\n")) {
        OperationsController::instance()->move(paths, destination);
        QGuiApplication::clipboard()->clear();       // what was cut can be pasted only once
    } else {
        OperationsController::instance()->copy(paths, destination);
    }
}
