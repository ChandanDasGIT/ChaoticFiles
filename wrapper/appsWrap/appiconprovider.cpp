#include "appiconprovider.h"

#include "apps/desktopapps.h"

#include <QIcon>
#include <QUrl>

namespace {

constexpr int kDefaultSize = 48;
const auto kFallbackIcon = QStringLiteral("application-x-executable");

QIcon findIcon(const QString& value)
{
    if (value.startsWith(QLatin1Char('/'))) return QIcon(value);
    QString name = value;
    if (name.endsWith(QLatin1String(".png")) || name.endsWith(QLatin1String(".svg")) || name.endsWith(QLatin1String(".xpm")))
        name.chop(4);
    return QIcon::fromTheme(name);
}

} // namespace

AppIconProvider::AppIconProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    // a desktop without a platform theme, like Hyprland, leaves Qt with no icon theme at all
    if (QIcon::themeName().isEmpty() || QIcon::themeName() == QLatin1String("hicolor")) {
        const std::string theme = DesktopApps::iconThemeName();
        if (!theme.empty()) QIcon::setThemeName(QString::fromStdString(theme));
    }
    QIcon::setFallbackSearchPaths(QIcon::fallbackSearchPaths() << QStringLiteral("/usr/share/pixmaps"));
}

QImage AppIconProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize)
{
    QIcon icon = findIcon(QUrl::fromPercentEncoding(id.toUtf8()));
    if (icon.isNull()) icon = QIcon::fromTheme(kFallbackIcon);

    const int edge = requestedSize.isValid() ? qMax(requestedSize.width(), requestedSize.height()) : kDefaultSize;
    QImage image = icon.pixmap(edge, edge).toImage();
    if (image.isNull()) {                  // nothing to show: an empty picture, so the row keeps its shape
        image = QImage(edge, edge, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
    }
    if (size) *size = image.size();
    return image;
}
