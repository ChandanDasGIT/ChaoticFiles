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
    Q_PROPERTY(int viewMode READ viewMode WRITE setViewMode NOTIFY viewChanged)               // 0 details, 1 grid
    Q_PROPERTY(int gridSize READ gridSize NOTIFY viewChanged)
    Q_PROPERTY(bool showThumbnails READ showThumbnails WRITE setShowThumbnails NOTIFY viewChanged)
    Q_PROPERTY(bool showPreview READ showPreview WRITE setShowPreview NOTIFY viewChanged)
    Q_PROPERTY(int windowButtons READ windowButtons WRITE setWindowButtons NOTIFY windowButtonsChanged)   // 0 automatic, 1 show, 2 hide
    Q_PROPERTY(bool windowButtonsVisible READ windowButtonsVisible NOTIFY windowButtonsChanged)
    Q_PROPERTY(bool localDefaults READ localDefaults WRITE setLocalDefaults NOTIFY localDefaultsChanged)  // default apps stay in this app

public:
    static SettingsController* instance();
    static SettingsController* create(QQmlEngine*, QJSEngine*);

    int themeMode() const { return m_themeMode; }
    void setThemeMode(int mode);
    bool systemDark() const { return m_systemTheme.isDark(); }
    bool showHidden() const { return m_showHidden; }
    void setShowHidden(bool show);
    int viewMode() const { return m_viewMode; }
    void setViewMode(int mode);
    int gridSize() const { return m_gridSize; }
    Q_INVOKABLE void zoomGrid(int steps);
    bool showThumbnails() const { return m_showThumbnails; }
    void setShowThumbnails(bool show);
    bool showPreview() const { return m_showPreview; }
    void setShowPreview(bool show);
    int windowButtons() const { return m_windowButtons; }
    void setWindowButtons(int mode);
    bool windowButtonsVisible() const;
    bool localDefaults() const;
    void setLocalDefaults(bool local);

signals:
    void themeModeChanged();
    void systemDarkChanged();
    void showHiddenChanged();
    void windowButtonsChanged();
    void viewChanged();
    void localDefaultsChanged();

private:
    explicit SettingsController(QObject* parent);

    QSettings m_settings;
    SystemTheme m_systemTheme;
    int m_themeMode = 0;
    bool m_showHidden = false;
    int m_windowButtons = 0;
    int m_viewMode = 0;
    int m_gridSize = 96;
    bool m_showThumbnails = true;
    bool m_showPreview = false;
};
