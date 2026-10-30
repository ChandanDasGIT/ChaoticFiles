#include "thumbnailprovider.h"

#include "pathbar/pathbar.h"
#include "thumbnails/thumbnails.h"

#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QQuickTextureFactory>
#include <QRunnable>
#include <QTemporaryDir>
#include <QUrl>
#include <atomic>
#include <sys/stat.h>

namespace {

constexpr int kCacheSize = 256;           // the "large" size of the shared cache
constexpr int kMaxThreads = 4;
constexpr qint64 kMaxImageBytes = 200 * 1024 * 1024;

QImage makeThumbnail(const QString& kind, const QString& path, const std::atomic<bool>& cancelled)
{
    if (kind == QLatin1String("image")) {
        if (QFile(path).size() > kMaxImageBytes) return {};
        QImageReader reader(path);
        reader.setAutoTransform(true);
        const QSize size = reader.size();
        if (size.isValid() && qMax(size.width(), size.height()) > kCacheSize)
            reader.setScaledSize(size.scaled(kCacheSize, kCacheSize, Qt::KeepAspectRatio));
        return reader.read();
    }

    QTemporaryDir folder;
    if (!folder.isValid()) return {};
    const QString output = folder.filePath(QStringLiteral("thumbnail.png"));
    const bool made = Thumbnails::generate(path.toStdString(), kind.toStdString(), output.toStdString(), kCacheSize,
                                           [&cancelled] { return cancelled.load(); });
    return made ? QImage(output) : QImage();
}

class ThumbnailResponse : public QQuickImageResponse, public QRunnable {
public:
    ThumbnailResponse(const QString& id, const QSize& requestedSize)
        : m_kind(id.section(QLatin1Char('/'), 0, 0))
        , m_path(QUrl::fromPercentEncoding(id.section(QLatin1Char('/'), 1).section(QLatin1Char('#'), 0, 0).toUtf8()))
        , m_requested(requestedSize)
    {
        setAutoDelete(false);
    }

    QQuickTextureFactory* textureFactory() const override { return QQuickTextureFactory::textureFactoryForImage(m_image); }
    void cancel() override { m_cancelled = true; }

    void run() override
    {
        if (!m_cancelled) m_image = load();
        if (m_requested.isValid() && !m_image.isNull())
            m_image = m_image.scaled(m_requested, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        emit finished();
    }

private:
    QImage load()
    {
        const std::string path = m_path.toStdString();
        struct stat info {};
        if (stat(path.c_str(), &info) != 0) return {};
        const QString mtime = QString::number(info.st_mtime);

        const QString cachePath = QString::fromStdString(Thumbnails::cachePath(path));
        QImage cached(cachePath);
        if (!cached.isNull() && cached.text(QStringLiteral("Thumb::MTime")) == mtime) return cached;

        QImage image = makeThumbnail(m_kind, m_path, m_cancelled);
        if (image.isNull()) return image;

        image.setText(QStringLiteral("Thumb::URI"), QString::fromStdString(PathBar::fileUrl(path)));
        image.setText(QStringLiteral("Thumb::MTime"), mtime);
        if (Thumbnails::ensureCacheFolder()) {
            const QString temporary = cachePath + QStringLiteral(".part");
            if (image.save(temporary, "PNG")) {
                QFile::remove(cachePath);
                QFile::rename(temporary, cachePath);
            }
        }
        return image;
    }

    QString m_kind;
    QString m_path;
    QSize m_requested;
    QImage m_image;
    std::atomic<bool> m_cancelled {false};
};

} // namespace

ThumbnailProvider::ThumbnailProvider()
{
    m_pool.setMaxThreadCount(kMaxThreads);
}

QQuickImageResponse* ThumbnailProvider::requestImageResponse(const QString& id, const QSize& requestedSize)
{
    auto* response = new ThumbnailResponse(id, requestedSize);
    m_pool.start(response);
    return response;
}
