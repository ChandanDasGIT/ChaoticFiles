#pragma once
#include <string>
#include <vector>

struct DesktopApp {
    std::string id;                        // "firefox.desktop"
    std::string name;                      // "Firefox"
    std::string exec;                      // the Exec= line
    std::string icon;                      // the Icon= line: a theme name or a full path
    bool terminal = false;
    bool noDisplay = false;                // NoDisplay=true: not in menus, but it still opens files
    std::vector<std::string> mimeTypes;
};

// Installed apps, from their .desktop files.
namespace DesktopApps {

std::vector<DesktopApp> all();                                 // sorted by name, without the NoDisplay ones
std::vector<DesktopApp> forMimeType(const std::string& mime);  // the default app first
std::vector<DesktopApp> forMimeTypes(const std::vector<std::string>& mimes);   // every app that opens any of them
// The apps mimeapps.list adds for this type ([Added Associations]) and takes away ([Removed Associations]).
struct Associations { std::vector<std::string> added, removed; };
Associations associationsFor(const std::vector<std::string>& mimes);
std::string defaultFor(const std::string& mime);               // its id, or that of what it is a kind of, or empty
std::vector<std::string> defaultsFor(const std::vector<std::string>& mimes);   // the same, many at once, in that order
bool setDefault(const std::string& appId, const std::vector<std::string>& mimes);
void keepDefaultsLocal(bool local);                            // defaults live in this app's own list; see LocalDefaults
bool defaultsAreLocal();
std::string openingType(const std::string& path);             // the type this file opens by, or empty
std::string iconThemeName();                                   // the one chosen in the GTK settings, or empty
std::string defaultNameFor(const std::string& mime);
DesktopApp parse(const std::string& id, const std::string& text);

// Turns an Exec= line and files into a command: handles quotes and %f %F %u %U, drops the other codes.
std::vector<std::string> commandFor(const std::string& exec, const std::vector<std::string>& paths);

bool open(const std::string& appId, const std::vector<std::string>& paths);
bool openWithDefault(const std::string& path);                 // false if the system should choose the app instead
// A compiled program you may run, AppImages included. Scripts are not: they open in an editor, as in Thunar.
bool isRunnableProgram(const std::string& path);
bool run(const std::string& path);                             // starts it in its own folder
bool openTerminal(const std::string& folder);

} // namespace DesktopApps
