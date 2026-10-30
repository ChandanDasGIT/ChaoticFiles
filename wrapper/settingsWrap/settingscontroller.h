#pragma once
#include "systemtheme.h"

#include <QObject>
#include <QSettings>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;
class QJSEngine;

// The app's settings (SettingsController in QML), kept in QSettings.
class SettingsController : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int themeMode READ themeMode WRITE setThemeMode NOTIFY themeModeChanged)       // 0 system, 1 dark, 2 light
    Q_PROPERTY(bool systemDark READ systemDark NOTIFY systemDarkChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)
    Q_PROPERTY(int windowButtons READ windowButtons WRITE setWindowButtons NOTIFY windowButtonsChanged)   // 0 automatic, 1 show, 2 hide
    Q_PROPERTY(bool windowButtonsVisible READ windowButtonsVisible NOTIFY windowButtonsChanged)

public:
    static SettingsController* instance();
    static SettingsController* create(QQmlEngine*, QJSEngine*);

    int themeMode() const { return m_themeMode; }
    void setThemeMode(int mode);
    bool systemDark() const { return m_systemTheme.isDark(); }
    bool showHidden() const { return m_showHidden; }
    void setShowHidden(bool show);
    int windowButtons() const { return m_windowButtons; }
    void setWindowButtons(int mode);
    bool windowButtonsVisible() const;

signals:
    void themeModeChanged();
    void systemDarkChanged();
    void showHiddenChanged();
    void windowButtonsChanged();

private:
    explicit SettingsController(QObject* parent);

    QSettings m_settings;
    SystemTheme m_systemTheme;
    int m_themeMode = 0;
    bool m_showHidden = false;
    int m_windowButtons = 0;
};
