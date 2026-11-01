#include "settingscontroller.h"

#include "apps/desktopapps.h"
#include "apps/localdefaults.h"

#include <QCoreApplication>
#include <QJSEngine>

namespace {

constexpr int kMinGridSize = 48;
constexpr int kMaxGridSize = 256;
constexpr int kGridStep = 16;

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
    , m_viewMode(m_settings.value("view/mode", 0).toInt())
    , m_gridSize(m_settings.value("view/gridSize", 96).toInt())
    , m_showThumbnails(m_settings.value("view/thumbnails", true).toBool())
    , m_showAlbumArt(m_settings.value("view/albumArt", true).toBool())
    , m_showPreview(m_settings.value("view/preview", false).toBool())
    , m_startMode(m_settings.value("start/mode", 0).toInt())
    , m_startFolder(m_settings.value("start/folder").toString())
{
    connect(&m_systemTheme, &SystemTheme::changed, this, &SettingsController::systemDarkChanged);
    LocalDefaults::seed();
    DesktopApps::keepDefaultsLocal(m_settings.value("apps/localDefaults", false).toBool());
}

bool SettingsController::localDefaults() const
{
    return DesktopApps::defaultsAreLocal();
}

void SettingsController::setLocalDefaults(bool local)
{
    if (local == localDefaults()) return;
    DesktopApps::keepDefaultsLocal(local);
    m_settings.setValue("apps/localDefaults", local);
    emit localDefaultsChanged();
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

void SettingsController::setStartMode(int mode)
{
    if (mode == m_startMode) return;
    m_startMode = mode;
    m_settings.setValue("start/mode", mode);
    emit startChanged();
}

void SettingsController::setStartFolder(const QString& folder)
{
    if (folder == m_startFolder) return;
    m_startFolder = folder;
    m_settings.setValue("start/folder", folder);
    emit startChanged();
}

QStringList SettingsController::startFolders() const
{
    QStringList folders;
    if (m_startMode == 1) folders = m_settings.value("start/lastTabs").toStringList();
    if (m_startMode == 2 && !m_startFolder.isEmpty()) folders << m_startFolder;
    return folders;
}

int SettingsController::startTab() const
{
    return m_startMode == 1 ? m_settings.value("start/lastTab", 0).toInt() : 0;
}

void SettingsController::rememberTabs(const QStringList& folders, int current)
{
    if (m_startMode != 1) return;
    m_settings.setValue("start/lastTabs", folders);
    m_settings.setValue("start/lastTab", current);
    m_settings.sync();
}

void SettingsController::setViewMode(int mode)
{
    if (mode == m_viewMode) return;
    m_viewMode = mode;
    m_settings.setValue("view/mode", mode);
    emit viewChanged();
}

void SettingsController::zoomGrid(int steps)
{
    const int size = qBound(kMinGridSize, m_gridSize + steps * kGridStep, kMaxGridSize);
    if (size == m_gridSize) return;
    m_gridSize = size;
    m_settings.setValue("view/gridSize", size);
    emit viewChanged();
}

void SettingsController::setShowThumbnails(bool show)
{
    if (show == m_showThumbnails) return;
    m_showThumbnails = show;
    m_settings.setValue("view/thumbnails", show);
    emit viewChanged();
}

void SettingsController::setShowAlbumArt(bool show)
{
    if (show == m_showAlbumArt) return;
    m_showAlbumArt = show;
    m_settings.setValue("view/albumArt", show);
    emit viewChanged();
}

void SettingsController::setShowPreview(bool show)
{
    if (show == m_showPreview) return;
    m_showPreview = show;
    m_settings.setValue("view/preview", show);
    emit viewChanged();
}

bool SettingsController::windowButtonsVisible() const
{
    if (m_windowButtons == 1) return true;
    if (m_windowButtons == 2) return false;
    return !isTilingDesktop();
}
