#include "apps/desktopapps.h"

#include "apps/localdefaults.h"
#include "mimetypes/mimetypes.h"
#include "pathbar/pathbar.h"
#include "process/process.h"
#include "sorting/sorting.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <dirent.h>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <thread>

namespace {

constexpr int kQueryThreads = 8;       // asking for a default runs a script, about 60 ms each

std::vector<std::string> applicationFolders()
{
    std::vector<std::string> folders;
    const char* dataHome = std::getenv("XDG_DATA_HOME");
    folders.push_back((dataHome && *dataHome ? std::string(dataHome) : PathBar::homeDirectory() + "/.local/share") + "/applications");
    const char* dataDirs = std::getenv("XDG_DATA_DIRS");
    std::istringstream dirs(dataDirs && *dataDirs ? dataDirs : "/usr/local/share:/usr/share");
    std::string dir;
    while (std::getline(dirs, dir, ':'))
        if (!dir.empty()) folders.push_back(dir + "/applications");
    return folders;
}

std::vector<std::string> split(const std::string& text, char separator)
{
    std::vector<std::string> parts;
    std::istringstream stream(text);
    std::string part;
    while (std::getline(stream, part, separator))
        if (!part.empty()) parts.push_back(part);
    return parts;
}

std::string firstLine(const std::string& text)
{
    std::string line = text.substr(0, text.find('\n'));
    line = line.substr(0, line.find(';'));          // "text/plain; charset=utf-8"
    while (!line.empty() && line.back() == ' ') line.pop_back();
    return line;
}

// the words of an Exec= line, with quotes removed
std::vector<std::string> execWords(const std::string& exec)
{
    std::vector<std::string> words;
    std::string word;
    bool quoted = false, inWord = false;
    for (std::size_t i = 0; i < exec.size(); ++i) {
        const char c = exec[i];
        if (quoted && c == '\\' && i + 1 < exec.size()) {
            word += exec[++i];
        } else if (c == '"') {
            quoted = !quoted;
            inWord = true;
        } else if (c == ' ' && !quoted) {
            if (inWord) words.push_back(word);
            word.clear();
            inWord = false;
        } else {
            word += c;
            inWord = true;
        }
    }
    if (inWord) words.push_back(word);
    return words;
}

bool isInstalled(const std::string& appId)
{
    std::error_code error;
    for (const std::string& folder : applicationFolders())
        if (std::filesystem::exists(folder + "/" + appId, error)) return true;
    return false;
}

std::string terminalProgram()
{
    if (const char* chosen = std::getenv("TERMINAL"); chosen && *chosen && !Process::findProgram(chosen).empty()) return chosen;
    for (const char* name : {"kitty", "foot", "alacritty", "ghostty", "wezterm", "konsole", "gnome-terminal", "xfce4-terminal", "xterm"})
        if (!Process::findProgram(name).empty()) return name;
    return {};
}

} // namespace

namespace DesktopApps {

DesktopApp parse(const std::string& id, const std::string& text)
{
    DesktopApp app;
    app.id = id;
    bool inEntry = false, hidden = false, isApplication = false;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty() && line.front() == '[') {
            inEntry = line == "[Desktop Entry]";
            continue;
        }
        if (!inEntry) continue;
        const auto equals = line.find('=');
        if (equals == std::string::npos) continue;
        const std::string key = line.substr(0, equals), value = line.substr(equals + 1);
        if (key == "Name") app.name = value;
        else if (key == "Exec") app.exec = value;
        else if (key == "Icon") app.icon = value;
        else if (key == "Type") isApplication = value == "Application";
        else if (key == "Terminal") app.terminal = value == "true";
        else if (key == "MimeType") app.mimeTypes = split(value, ';');
        else if ((key == "NoDisplay" || key == "Hidden") && value == "true") hidden = true;
    }
    if (hidden || !isApplication || app.exec.empty() || app.name.empty()) app.exec.clear();
    return app;
}

std::vector<DesktopApp> all()
{
    std::map<std::string, DesktopApp> found;
    for (const std::string& folder : applicationFolders()) {
        DIR* dir = opendir(folder.c_str());
        if (!dir) continue;
        while (dirent* item = readdir(dir)) {
            const std::string name = item->d_name;
            if (name.size() < 9 || name.compare(name.size() - 8, 8, ".desktop") != 0 || found.count(name)) continue;
            std::ifstream file(folder + "/" + name);
            std::stringstream text;
            text << file.rdbuf();
            found[name] = parse(name, text.str());       // kept even if hidden: it hides later folders' copies
        }
        closedir(dir);
    }
    std::vector<DesktopApp> apps;
    for (auto& [id, app] : found)
        if (!app.exec.empty()) apps.push_back(std::move(app));
    std::sort(apps.begin(), apps.end(), [](const DesktopApp& a, const DesktopApp& b) { return naturalCompare(a.name, b.name) < 0; });
    return apps;
}

void keepDefaultsLocal(bool local)
{
    LocalDefaults::setInUse(local);
}

bool defaultsAreLocal()
{
    return LocalDefaults::inUse();
}

std::string defaultFor(const std::string& mime)
{
    const bool local = defaultsAreLocal();
    // a type with no default of its own goes by what it is a kind of: QML is a kind of plain text
    for (const std::string& type : MimeTypes::chain(mime)) {
        // a type this app's list doesn't name, or names only apps since removed, follows the system
        if (local)
            for (const std::string& id : LocalDefaults::appsFor(type))
                if (isInstalled(id)) return id;
        const std::string id = firstLine(Process::run({"xdg-mime", "query", "default", type}).output);
        if (!id.empty()) return id;
    }
    return {};
}

std::vector<std::string> defaultsFor(const std::vector<std::string>& mimes)
{
    std::vector<std::string> ids(mimes.size());
    std::atomic<std::size_t> next {0};
    std::vector<std::thread> threads;
    for (int i = 0; i < kQueryThreads; ++i) {
        threads.emplace_back([&] {
            for (std::size_t at = next++; at < mimes.size(); at = next++) ids[at] = defaultFor(mimes[at]);
        });
    }
    for (std::thread& thread : threads) thread.join();
    return ids;
}

bool setDefault(const std::string& appId, const std::vector<std::string>& mimes)
{
    if (mimes.empty()) return false;
    if (defaultsAreLocal()) return LocalDefaults::set(appId, mimes);
    std::vector<std::string> command {"xdg-mime", "default", appId};
    command.insert(command.end(), mimes.begin(), mimes.end());
    return Process::run(command).exitCode == 0;
}

// The name comes first and the content only settles what the name can't, as the standard says. Asking xdg-mime
// alone is not enough: on a desktop with no file manager of its own it asks `file`, which calls a QML file plain text.
std::string openingType(const std::string& path)
{
    const auto content = [&path] { return firstLine(Process::run({"xdg-mime", "query", "filetype", path}).output); };
    std::error_code error;
    if (std::filesystem::is_directory(path, error)) return content();

    const std::vector<std::string> types = MimeTypes::candidates(std::filesystem::path(path).filename().string());
    if (types.size() == 1) return types.front();
    const std::string found = content();
    for (const std::string& type : types) {
        const std::vector<std::string> kinds = MimeTypes::chain(found);
        if (std::find(kinds.begin(), kinds.end(), type) != kinds.end()) return type;
    }
    if (found.rfind("text/", 0) == 0)                                 // a .ts file with text in it, not video
        for (const std::string& type : types)
            if (type.rfind("text/", 0) == 0) return type;
    return found.empty() && !types.empty() ? types.front() : found;
}

std::string iconThemeName()
{
    const char* config = std::getenv("XDG_CONFIG_HOME");
    const std::string base = config && *config ? config : PathBar::homeDirectory() + "/.config";
    for (const char* version : {"gtk-3.0", "gtk-4.0"}) {
        std::ifstream file(base + "/" + version + "/settings.ini");
        std::string line;
        while (std::getline(file, line)) {
            const auto equals = line.find('=');
            if (equals == std::string::npos || line.rfind("gtk-icon-theme-name", 0) != 0) continue;
            std::string name = line.substr(equals + 1);
            std::erase_if(name, [](char c) { return c == ' ' || c == '"' || c == '\r'; });
            if (!name.empty()) return name;
        }
    }
    return {};
}

std::string defaultNameFor(const std::string& mime)
{
    const std::string id = defaultFor(mime);
    if (id.empty()) return {};
    for (const DesktopApp& app : all())
        if (app.id == id) return app.name;
    return id;
}

std::vector<DesktopApp> forMimeType(const std::string& mime)
{
    return forMimeTypes({mime});
}

std::vector<DesktopApp> forMimeTypes(const std::vector<std::string>& mimes)
{
    if (mimes.empty()) return {};
    const std::string chosen = defaultFor(mimes.front());
    std::vector<std::string> wanted;
    for (const std::string& mime : mimes) {
        const std::vector<std::string> kinds = MimeTypes::related(mime);
        wanted.insert(wanted.end(), kinds.begin(), kinds.end());
    }
    std::vector<DesktopApp> apps;
    for (DesktopApp& app : all()) {
        const auto& types = app.mimeTypes;
        const bool opens = app.id == chosen || std::any_of(wanted.begin(), wanted.end(), [&types](const std::string& mime) {
            return std::find(types.begin(), types.end(), mime) != types.end();
        });
        if (!opens) continue;
        if (app.id == chosen) apps.insert(apps.begin(), std::move(app));
        else apps.push_back(std::move(app));
    }
    return apps;
}

std::vector<std::string> commandFor(const std::string& exec, const std::vector<std::string>& paths)
{
    std::vector<std::string> command;
    bool placed = false;
    for (const std::string& word : execWords(exec)) {
        if (word == "%f" || word == "%F" || word == "%u" || word == "%U") {
            const bool asUrl = word == "%u" || word == "%U";
            const bool many = word == "%F" || word == "%U";
            for (std::size_t i = 0; i < paths.size() && (many || i == 0); ++i)
                command.push_back(asUrl ? PathBar::fileUrl(paths[i]) : paths[i]);
            placed = true;
        } else if (word.size() == 2 && word[0] == '%') {
            continue;                                  // %i %c %k and old codes
        } else {
            std::string clean;
            for (std::size_t i = 0; i < word.size(); ++i) {
                if (word[i] == '%' && i + 1 < word.size()) {
                    if (word[i + 1] == '%') clean += '%';
                    ++i;
                } else {
                    clean += word[i];
                }
            }
            command.push_back(clean);
        }
    }
    if (!placed) command.insert(command.end(), paths.begin(), paths.end());
    return command;
}

bool open(const std::string& appId, const std::vector<std::string>& paths)
{
    for (const DesktopApp& app : all()) {
        if (app.id != appId) continue;
        std::vector<std::string> command = commandFor(app.exec, paths);
        if (app.terminal) {
            const std::string terminal = terminalProgram();
            if (terminal.empty()) return false;
            command.insert(command.begin(), {terminal, "-e"});
        }
        return Process::startDetached(command, paths.empty() ? std::string() : PathBar::parent(paths.front()));
    }
    return false;
}

bool openWithDefault(const std::string& path)
{
    const std::string id = defaultFor(openingType(path));
    return !id.empty() && open(id, {path});
}

bool openTerminal(const std::string& folder)
{
    const std::string terminal = terminalProgram();
    return !terminal.empty() && Process::startDetached({terminal}, folder);
}

} // namespace DesktopApps
