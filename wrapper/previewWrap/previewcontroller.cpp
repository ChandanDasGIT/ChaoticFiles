#include "previewcontroller.h"

#include "magic/magicfile.h"

#include <QFile>
#include <QImageIOHandler>
#include <QImageReader>
#include <QUrl>

namespace {
constexpr qint64 kPreviewBytes = 16 * 1024;
}

PreviewController::PreviewController(QObject* parent)
    : QObject(parent)
{
}

void PreviewController::clear()
{
    ++m_generation;
    m_job.cancel();
    if (m_text.isEmpty()) return;
    m_text.clear();
    emit textChanged();
}

void PreviewController::showText(const QString& path)
{
    clear();
    const quint64 generation = m_generation;
    m_job.start([this, path, generation](const BackgroundJob::Handle& handle) {
        if (MagicFile::encoding(path.toStdString()) == "binary") return;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) return;
        const QString text = QString::fromUtf8(file.read(kPreviewBytes));
        handle.post([this, text, generation] {
            QMetaObject::invokeMethod(this, [this, text, generation] {
                if (generation != m_generation) return;
                m_text = text;
                emit textChanged();
            }, Qt::QueuedConnection);
        });
    });
}

QVariantMap PreviewController::imageDetails(const QString& path) const
{
    QImageReader reader(path);
    QSize size = reader.size();
    if (reader.transformation() & QImageIOHandler::TransformationRotate90) size.transpose();
    return {
        {"url", QUrl::fromLocalFile(path)},
        {"animated", reader.supportsAnimation()},
        {"width", size.width()},
        {"height", size.height()},
    };
}
