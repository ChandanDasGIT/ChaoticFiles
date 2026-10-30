#pragma once
#include <map>
#include <string>
#include <vector>

struct ShortcutAction {
    std::string id;                        // "copy"
    std::string title;                     // "Copy"
    std::vector<std::string> defaults;     // key sequences as text: "Ctrl+C"
};

// Which keys do what. A key belongs to one action only: giving it to another takes it from the first.
class ShortcutMap {
public:
    ShortcutMap();

    const std::vector<ShortcutAction>& actions() const { return m_actions; }
    const std::vector<std::string>& keysOf(const std::string& id) const;

    void set(const std::string& id, const std::string& key);       // an empty key leaves the action with none
    void resetAll();
    bool isCustom(const std::string& id) const;
    const std::map<std::string, std::vector<std::string>>& custom() const { return m_custom; }
    void restore(const std::string& id, const std::vector<std::string>& keys);   // a saved choice

private:
    std::vector<ShortcutAction> m_actions;
    std::map<std::string, std::vector<std::string>> m_keys;
    std::map<std::string, std::vector<std::string>> m_custom;
};
