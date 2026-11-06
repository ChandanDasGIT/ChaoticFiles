#pragma once
#include <QObject>
#include <QPointer>
#include <QQuickItem>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QMenu;

// A real desktop menu (a QMenu) for QML (ContextMenu in QML). Like the menus of other apps it opens at the
// pointer, can reach past the edge of the window, and the desktop keeps it on screen. QML hands it the rows
// and the colours and sizes of the theme; it says which row was chosen, and QML runs that row's action.
class ContextMenu : public QObject {
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ContextMenu(QObject* parent = nullptr);
    ~ContextMenu() override;

    // `entries` as PopupMenu documents them; `style` holds the theme's colours and sizes, see PopupMenu.qml.
    // `item` is any item of the window, and x, y a point in that window.
    Q_INVOKABLE void popup(QQuickItem* item, qreal x, qreal y, const QVariantList& entries, const QVariantMap& style);
    Q_INVOKABLE void close();

signals:
    void triggered(int index);           // the row of `entries` that was chosen
    void closed();

private:
    QPointer<QMenu> m_menu;
};
