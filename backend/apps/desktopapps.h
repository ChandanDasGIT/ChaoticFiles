#pragma once
#include <string>
#include <vector>

struct DesktopApp {
    std::string id;                        // "firefox.desktop"
    std::string name;                      // "Firefox"
    std::string exec;                      // the Exec= line
    bool terminal = false;
    std::vector<std::string> mimeTypes;
};

// Installed apps, from their .desktop files.
namespace DesktopApps {

std::vector<DesktopApp> all();                                 // sorted by name
std::vector<DesktopApp> forMimeType(const std::string& mime);  // the default app first
std::string defaultFor(const std::string& mime);               // its id, or empty
std::string defaultNameFor(const std::string& mime);
DesktopApp parse(const std::string& id, const std::string& text);

// Turns an Exec= line and files into a command: handles quotes and %f %F %u %U, drops the other codes.
std::vector<std::string> commandFor(const std::string& exec, const std::vector<std::string>& paths);

bool open(const std::string& appId, const std::vector<std::string>& paths);
bool openTerminal(const std::string& folder);

} // namespace DesktopApps
