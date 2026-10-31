#pragma once
#include <string>
#include <vector>

struct DesktopApp {
    std::string id;                        // "firefox.desktop"
    std::string name;                      // "Firefox"
    std::string exec;                      // the Exec= line
    std::string icon;                      // the Icon= line: a theme name or a full path
    bool terminal = false;
    std::vector<std::string> mimeTypes;
};

// Installed apps, from their .desktop files.
namespace DesktopApps {

std::vector<DesktopApp> all();                                 // sorted by name
std::vector<DesktopApp> forMimeType(const std::string& mime);  // the default app first
std::vector<DesktopApp> forMimeTypes(const std::vector<std::string>& mimes);   // every app that opens any of them
std::string defaultFor(const std::string& mime);               // its id, or empty
std::vector<std::string> defaultsFor(const std::vector<std::string>& mimes);   // the same, many at once, in that order
bool setDefault(const std::string& appId, const std::vector<std::string>& mimes);
void keepDefaultsLocal(bool local);                            // defaults live in this app's own list; see LocalDefaults
bool defaultsAreLocal();
std::string openingType(const std::string& path);             // the type xdg-open goes by for this file, or empty
std::string iconThemeName();                                   // the one chosen in the GTK settings, or empty
std::string defaultNameFor(const std::string& mime);
DesktopApp parse(const std::string& id, const std::string& text);

// Turns an Exec= line and files into a command: handles quotes and %f %F %u %U, drops the other codes.
std::vector<std::string> commandFor(const std::string& exec, const std::vector<std::string>& paths);

bool open(const std::string& appId, const std::vector<std::string>& paths);
bool openWithLocalDefault(const std::string& path);            // false if the system should choose the app instead
bool openTerminal(const std::string& folder);

} // namespace DesktopApps
