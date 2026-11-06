#include "contextmenu.h"

#include <QFontMetrics>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QPainter>
#include <QQuickItem>
#include <QQuickWindow>
#include <QWidgetAction>

namespace {

QString css(const QColor& color)
{
    return QStringLiteral("rgba(%1,%2,%3,%4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
}

// an icon of the icon font, drawn in one colour for normal rows and another for disabled ones
QIcon glyphIcon(const QString& glyph, const QVariantMap& style, qreal ratio)
{
    QFont font(style.value("iconFont").toString());
    font.setPixelSize(style.value("iconSize").toInt() - 1);
    const int side = style.value("iconSize").toInt() + 4;

    QIcon icon;
    const auto draw = [&](const QColor& color, QIcon::Mode mode) {
        QPixmap pixmap(QSize(side, side) * ratio);
        pixmap.setDevicePixelRatio(ratio);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::TextAntialiasing);
        painter.setFont(font);
        painter.setPen(color);
        painter.drawText(QRect(0, 0, side, side), Qt::AlignCenter, glyph);
        painter.end();
        icon.addPixmap(pixmap, mode);
    };
    draw(style.value("iconColor").value<QColor>(), QIcon::Normal);
    draw(style.value("iconColor").value<QColor>(), QIcon::Active);
    draw(style.value("disabledColor").value<QColor>(), QIcon::Disabled);
    return icon;
}

QString styleSheet(const QVariantMap& s, int itemVerticalPadding)
{
    const auto c = [&s](const char* key) { return css(s.value(key).value<QColor>()); };
    const auto n = [&s](const char* key) { return s.value(key).toInt(); };
    return QStringLiteral(
        "QMenu { background: %1; border: 1px solid %2; border-radius: %3px; padding: %4px; min-width: %5px; }"
        "QMenu::item { color: %6; background: transparent; border-radius: %7px; padding: %8px %9px %8px %9px; }"
        "QMenu::item:selected { background: %10; }"
        "QMenu::item:disabled { color: %11; background: transparent; }"
        "QMenu::icon { padding-left: %9px; }"
        "QMenu::separator { height: 1px; background: %12; margin: %4px 0px; }"
        "QLabel#menuHeader { color: %13; font-size: %14px; padding: 0px %9px; min-height: %15px; background: transparent; }")
        .arg(c("background"), c("border")).arg(n("radius")).arg(n("padding")).arg(n("minWidth"))
        .arg(c("textColor")).arg(n("rowRadius")).arg(itemVerticalPadding).arg(n("rowPadding"))
        .arg(c("hoverColor"), c("disabledColor"), c("dividerColor"), c("headerColor"))
        .arg(n("smallFontSize")).arg(n("rowHeight"));
}

} // namespace

ContextMenu::ContextMenu(QObject* parent)
    : QObject(parent)
{
}

ContextMenu::~ContextMenu()
{
    if (m_menu) disconnect(m_menu, nullptr, this, nullptr);
    delete m_menu;
}

void ContextMenu::close()
{
    if (m_menu) m_menu->close();
}

void ContextMenu::popup(QQuickItem* item, qreal x, qreal y, const QVariantList& entries, const QVariantMap& style)
{
    QQuickWindow* window = item ? item->window() : nullptr;
    if (!window) return;
    if (m_menu) {                                       // a menu still open is replaced, without a word about it
        disconnect(m_menu, nullptr, this, nullptr);
        delete m_menu;
    }

    auto* menu = new QMenu;
    m_menu = menu;
    // the rounded corners need a see-through window; the theme draws the border, not the desktop
    menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    menu->setAttribute(Qt::WA_TranslucentBackground);
    QFont font = menu->font();
    font.setPixelSize(style.value("fontSize").toInt());
    menu->setFont(font);
    const int textHeight = QFontMetrics(font).height();
    menu->setStyleSheet(styleSheet(style, std::max(0, (style.value("rowHeight").toInt() - textHeight) / 2)));

    const QString check = style.value("checkGlyph").toString();
    for (int i = 0; i < entries.size(); ++i) {
        const QVariantMap entry = entries.at(i).toMap();
        if (entry.value("separator").toBool()) {
            menu->addSeparator();
        } else if (entry.contains("header")) {
            auto* label = new QLabel(entry.value("header").toString());
            label->setObjectName("menuHeader");
            auto* action = new QWidgetAction(menu);
            action->setDefaultWidget(label);
            action->setEnabled(false);
            menu->addAction(action);
        } else {
            // text after a tab is shown on the right, where a shortcut goes; no real shortcut is made
            QString text = entry.value("text").toString();
            text.replace('&', "&&");
            const QString right = entry.value("checked").toBool() ? check : entry.value("shortcut").toString();
            if (!right.isEmpty()) text += '\t' + right;
            QAction* action = menu->addAction(text);
            action->setData(i);
            action->setEnabled(entry.value("enabled", true).toBool());
            const QString glyph = entry.value("icon").toString();
            if (!glyph.isEmpty()) action->setIcon(glyphIcon(glyph, style, window->devicePixelRatio()));
        }
    }

    connect(menu, &QMenu::triggered, this, [this](QAction* action) {
        if (action->data().isValid()) emit triggered(action->data().toInt());
    });
    connect(menu, &QMenu::aboutToHide, this, [this, menu] {
        menu->deleteLater();
        emit closed();
    });

    // belongs to the app's window, so the desktop places it against that window (needed on Wayland)
    menu->winId();
    menu->windowHandle()->setTransientParent(window);
    menu->popup(window->mapToGlobal(QPointF(x, y)).toPoint());
}
