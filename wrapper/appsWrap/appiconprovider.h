#pragma once
#include <QQuickImageProvider>

// App icons for QML: image://appicon/<the Icon= value of a .desktop file>, percent-encoded.
class AppIconProvider : public QQuickImageProvider {
public:
    AppIconProvider();
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
};
