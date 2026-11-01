#include "places/places.h"

#include "pathbar/pathbar.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

namespace {

// mountinfo writes spaces and a few other characters as \040 style octal
std::string unescape(const std::string& text)
{
    std::string out;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 3 < text.size()) {
            out += static_cast<char>((text[i + 1] - '0') * 64 + (text[i + 2] - '0') * 8 + (text[i + 3] - '0'));
            i += 3;
        } else {
            out += text[i];
        }
    }
    return out;
}

// reads ~/.config/user-dirs.dirs: XDG_MUSIC_DIR="$HOME/Music"
std::map<std::string, std::string> readUserDirs(const std::string& home)
{
    std::map<std::string, std::string> dirs;
    const char* config = std::getenv("XDG_CONFIG_HOME");
    std::ifstream file((config && *config ? std::string(config) : home + "/.config") + "/user-dirs.dirs");

    std::string line;
    while (std::getline(file, line)) {
        const auto equals = line.find('=');
        if (line.rfind("XDG_", 0) != 0 || equals == std::string::npos) continue;
        std::string value = line.substr(equals + 1);
        if (value.size() < 2 || value.front() != '"' || value.back() != '"') continue;
        value = value.substr(1, value.size() - 2);
        if (value.rfind("$HOME", 0) == 0) value = home + value.substr(5);
        dirs[line.substr(0, equals)] = value;
    }
    return dirs;
}

bool isSystemMount(const std::string& path)
{
    static const std::vector<std::string> system = {"/", "/boot", "/home", "/usr", "/var", "/nix", "/snap", "/opt", "/root", "/tmp", "/srv"};
    for (const std::string& dir : system)
        if (path == dir || (dir != "/" && path.rfind(dir + "/", 0) == 0)) return true;
    return false;
}

} // namespace

namespace Places {

std::vector<Place> userPlaces()
{
    const std::string home = PathBar::homeDirectory();
    const auto userDirs = readUserDirs(home);

    std::vector<Place> places {{"Home", home, "home"}, {"Recent", PathBar::kRecentLocation, "recent"}};
    const struct { const char* key; const char* name; const char* kind; } standard[] = {
        {"XDG_DESKTOP_DIR", "Desktop", "desktop"},
        {"XDG_DOCUMENTS_DIR", "Documents", "documents"},
        {"XDG_DOWNLOAD_DIR", "Downloads", "downloads"},
        {"XDG_MUSIC_DIR", "Music", "music"},
        {"XDG_PICTURES_DIR", "Pictures", "pictures"},
        {"XDG_VIDEOS_DIR", "Videos", "videos"},
    };
    for (const auto& dir : standard) {
        const auto it = userDirs.find(dir.key);
        const std::string path = PathBar::normalize(it != userDirs.end() ? it->second : home + "/" + dir.name);
        if (path != home && PathBar::isDirectory(path)) places.push_back({dir.name, path, dir.kind});
    }
    places.push_back({"File System", "/", "root"});
    places.push_back({"Trash", PathBar::kTrashLocation, "trash"});
    return places;
}

Place driveFromMountInfo(const std::string& line)
{
    // 36 25 0:32 / /run/media/me/USB rw,nosuid shared:1 - ext4 /dev/sdb1 rw
    const auto separator = line.find(" - ");
    if (separator == std::string::npos) return {};

    std::istringstream before(line.substr(0, separator));
    std::string id, parent, device, root, mountPoint;
    before >> id >> parent >> device >> root >> mountPoint;

    std::istringstream after(line.substr(separator + 3));
    std::string type, source;
    after >> type >> source;

    mountPoint = unescape(mountPoint);
    if (source.rfind("/dev/", 0) != 0 || source.rfind("/dev/loop", 0) == 0 || isSystemMount(mountPoint)) return {};
    return {PathBar::baseName(mountPoint), mountPoint, "drive", source};
}

std::vector<Place> drives()
{
    std::vector<Place> found;
    std::ifstream file("/proc/self/mountinfo");
    std::string line;
    while (std::getline(file, line)) {
        Place drive = driveFromMountInfo(line);
        if (!drive.path.empty()) found.push_back(std::move(drive));
    }
    std::sort(found.begin(), found.end(), [](const Place& a, const Place& b) { return a.path < b.path; });
    found.erase(std::unique(found.begin(), found.end(), [](const Place& a, const Place& b) { return a.path == b.path; }),
                found.end());
    return found;
}

std::vector<std::string> placePins(const std::vector<std::string>& pins, const std::vector<std::string>& moved,
                                   std::size_t index)
{
    const auto isMoved = [&moved](const std::string& path) { return std::find(moved.begin(), moved.end(), path) != moved.end(); };

    std::vector<std::string> placed;
    std::size_t at = 0;
    for (std::size_t i = 0; i < pins.size(); ++i) {
        if (i == index) at = placed.size();
        if (!isMoved(pins[i])) placed.push_back(pins[i]);
    }
    if (index >= pins.size()) at = placed.size();

    std::vector<std::string> fresh;
    for (const std::string& path : moved)
        if (std::find(fresh.begin(), fresh.end(), path) == fresh.end()) fresh.push_back(path);
    placed.insert(placed.begin() + static_cast<std::ptrdiff_t>(at), fresh.begin(), fresh.end());
    return placed;
}

} // namespace Places
