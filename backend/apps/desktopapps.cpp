#include "apps/desktopapps.h"

#include "pathbar/pathbar.h"
#include "process/process.h"
#include "sorting/sorting.h"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <map>
#include <sstream>

namespace {

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

std::string defaultFor(const std::string& mime)
{
    const auto result = Process::run({"xdg-mime", "query", "default", mime});
    std::string id = result.output;
    while (!id.empty() && (id.back() == '\n' || id.back() == ' ')) id.pop_back();
    return id;
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
    const std::string chosen = defaultFor(mime);
    const bool text = mime.rfind("text/", 0) == 0;
    std::vector<DesktopApp> apps;
    for (DesktopApp& app : all()) {
        const auto& types = app.mimeTypes;
        const bool opens = std::find(types.begin(), types.end(), mime) != types.end()
                        || (text && std::find(types.begin(), types.end(), "text/plain") != types.end())
                        || app.id == chosen;
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

bool openTerminal(const std::string& folder)
{
    const std::string terminal = terminalProgram();
    return !terminal.empty() && Process::startDetached({terminal}, folder);
}

} // namespace DesktopApps
