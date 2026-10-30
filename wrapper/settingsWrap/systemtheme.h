#pragma once
#include <QDBusVariant>
#include <QObject>

// Whether the desktop prefers dark. Qt can't tell on every desktop (on Hyprland it says "unknown"), so this
// asks the freedesktop settings portal first, and listens for changes. Without a portal, Qt's answer is used.
class SystemTheme : public QObject {
    Q_OBJECT

public:
    explicit SystemTheme(QObject* parent = nullptr);

    bool isDark() const { return m_dark; }

signals:
    void changed();

private slots:
    void settingChanged(const QString& group, const QString& key, const QDBusVariant& value);

private:
    void applyPortalValue(const QVariant& value);
    void applyQtValue();
    void setDark(bool dark);

    bool m_dark = true;
    bool m_portalAnswered = false;
};
