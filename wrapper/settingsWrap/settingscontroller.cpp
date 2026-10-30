#include "settingscontroller.h"

#include <QCoreApplication>
#include <QJSEngine>

namespace {

// Desktops that tile windows have no use for minimise and maximise buttons.
bool isTilingDesktop()
{
    static const QStringList tiling = {"hyprland", "sway", "i3", "niri", "river", "bspwm", "dwm", "awesome", "qtile"};
    const QStringList desktops = qEnvironmentVariable("XDG_CURRENT_DESKTOP").toLower().split(QLatin1Char(':'));
    for (const QString& desktop : desktops)
        if (tiling.contains(desktop)) return true;
    return false;
}

} // namespace

SettingsController* SettingsController::instance()
{
    static SettingsController* controller = new SettingsController(QCoreApplication::instance());
    return controller;
}

SettingsController* SettingsController::create(QQmlEngine*, QJSEngine*)
{
    SettingsController* controller = instance();
    QJSEngine::setObjectOwnership(controller, QJSEngine::CppOwnership);
    return controller;
}

SettingsController::SettingsController(QObject* parent)
    : QObject(parent)
    , m_themeMode(m_settings.value("theme/mode", 0).toInt())
    , m_showHidden(m_settings.value("view/showHidden", false).toBool())
    , m_windowButtons(m_settings.value("window/buttons", 0).toInt())
{
    connect(&m_systemTheme, &SystemTheme::changed, this, &SettingsController::systemDarkChanged);
}

void SettingsController::setThemeMode(int mode)
{
    if (mode == m_themeMode) return;
    m_themeMode = mode;
    m_settings.setValue("theme/mode", mode);
    emit themeModeChanged();
}

void SettingsController::setShowHidden(bool show)
{
    if (show == m_showHidden) return;
    m_showHidden = show;
    m_settings.setValue("view/showHidden", show);
    emit showHiddenChanged();
}

void SettingsController::setWindowButtons(int mode)
{
    if (mode == m_windowButtons) return;
    m_windowButtons = mode;
    m_settings.setValue("window/buttons", mode);
    emit windowButtonsChanged();
}

bool SettingsController::windowButtonsVisible() const
{
    if (m_windowButtons == 1) return true;
    if (m_windowButtons == 2) return false;
    return !isTilingDesktop();
}

QStringList SettingsController::savedTabs() const
{
    return m_settings.value("session/tabs").toStringList();
}

int SettingsController::savedCurrentTab() const
{
    return m_settings.value("session/current", 0).toInt();
}

void SettingsController::saveTabs(const QStringList& paths, int current)
{
    m_settings.setValue("session/tabs", paths);
    m_settings.setValue("session/current", current);
}
