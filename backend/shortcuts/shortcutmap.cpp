#include "shortcuts/shortcutmap.h"

#include <algorithm>

ShortcutMap::ShortcutMap()
    : m_actions {
        {"newTab", "New tab", {"Ctrl+T"}},
        {"closeTab", "Close tab", {"Ctrl+W"}},
        {"nextTab", "Next tab", {"Ctrl+Tab", "Ctrl+PgDown"}},
        {"previousTab", "Previous tab", {"Ctrl+Shift+Tab", "Ctrl+PgUp"}},
        {"reopenTab", "Reopen closed tab", {"Ctrl+Shift+T"}},
        {"address", "Edit address", {"Ctrl+L", "Alt+D"}},
        {"back", "Back", {"Alt+Left"}},
        {"forward", "Forward", {"Alt+Right"}},
        {"up", "Up one folder", {"Alt+Up"}},
        {"refresh", "Refresh", {"F5", "Ctrl+R"}},
        {"search", "Search", {"Ctrl+F"}},
        {"showHidden", "Show hidden files", {"Ctrl+H"}},
        {"detailsView", "Details view", {"Ctrl+1"}},
        {"gridView", "Grid view", {"Ctrl+2"}},
        {"preview", "Preview panel", {"F3"}},
        {"quickLook", "Quick look at an image", {"Space"}},
        {"properties", "Properties", {"Alt+Return", "Alt+Enter"}},
        {"copy", "Copy", {"Ctrl+C"}},
        {"cut", "Cut", {"Ctrl+X"}},
        {"paste", "Paste", {"Ctrl+V"}},
        {"undo", "Undo", {"Ctrl+Z"}},
        {"rename", "Rename", {"F2"}},
        {"trash", "Move to Trash", {"Del"}},
        {"deleteForever", "Delete permanently", {"Shift+Del"}},
        {"newFolder", "New folder", {"Ctrl+Shift+N"}},
        {"terminal", "Open terminal here", {"F4"}},
        {"settings", "Settings", {"Ctrl+,"}},
        {"quit", "Quit", {"Ctrl+Q"}},
    }
{
    resetAll();
}

const std::vector<std::string>& ShortcutMap::keysOf(const std::string& id) const
{
    static const std::vector<std::string> none;
    const auto it = m_keys.find(id);
    return it == m_keys.end() ? none : it->second;
}

void ShortcutMap::restore(const std::string& id, const std::vector<std::string>& keys)
{
    if (!m_keys.count(id)) return;
    m_keys[id] = keys;
    m_custom[id] = keys;
}

void ShortcutMap::set(const std::string& id, const std::string& key)
{
    if (!m_keys.count(id)) return;
    for (auto& [otherId, keys] : m_keys) {
        if (otherId == id) continue;
        const auto place = std::find(keys.begin(), keys.end(), key);
        if (place != keys.end()) {
            keys.erase(place);
            m_custom[otherId] = keys;
        }
    }
    m_keys[id] = key.empty() ? std::vector<std::string>() : std::vector<std::string> {key};
    m_custom[id] = m_keys[id];
}

void ShortcutMap::resetAll()
{
    m_keys.clear();
    m_custom.clear();
    for (const ShortcutAction& action : m_actions) m_keys[action.id] = action.defaults;
}

bool ShortcutMap::isCustom(const std::string& id) const
{
    return m_custom.count(id) > 0;
}
