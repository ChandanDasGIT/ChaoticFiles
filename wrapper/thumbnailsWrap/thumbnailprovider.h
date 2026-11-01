#pragma once
#include <QQuickAsyncImageProvider>
#include <QThreadPool>

// Thumbnails for QML, as image://thumb/<kind>/<encoded path>#<date>. They are made off the UI thread, a few at a time,
// and kept in the shared thumbnail cache. <kind> is "image", "video", "pdf" or
// "audio", which is a song's album art.
class ThumbnailProvider : public QQuickAsyncImageProvider {
public:
    ThumbnailProvider();

    QQuickImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize) override;

private:
    QThreadPool m_pool;
};
