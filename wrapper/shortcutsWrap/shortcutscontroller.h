#pragma once
#include "shortcuts/shortcutmap.h"

#include <QObject>
#include <QSettings>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

// Keyboard shortcuts (ShortcutsController in QML): the keys of each action, and the user's changes to them.
class ShortcutsController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QVariantMap keys READ keys NOTIFY changed)          // id to a list of key sequences
    Q_PROPERTY(QVariantList actions READ actions NOTIFY changed)   // [{id, title, keys: "Ctrl+C"}]

public:
    explicit ShortcutsController(QObject* parent = nullptr);

    QVariantMap keys() const;
    QVariantList actions() const;

    Q_INVOKABLE QString label(const QString& id) const;            // the first key, for a menu
    Q_INVOKABLE void set(const QString& id, const QString& key);
    Q_INVOKABLE void resetAll();
    Q_INVOKABLE QString sequenceFor(int key, int modifiers) const; // "" for a modifier on its own

signals:
    void changed();

private:
    void save();

    ShortcutMap m_map;
    QSettings m_settings;
};
