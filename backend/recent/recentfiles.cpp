#include "recent/recentfiles.h"

#include "pathbar/pathbar.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

namespace {

constexpr std::size_t kMaxItems = 200;

std::string dataHome()
{
    const char* data = std::getenv("XDG_DATA_HOME");
    return data && *data ? data : PathBar::homeDirectory() + "/.local/share";
}

std::string ownFile() { return dataHome() + "/ChaoticFiles/recent"; }

std::vector<std::string> readOwn()
{
    std::vector<std::string> paths;
    std::ifstream file(ownFile());
    std::string line;
    while (std::getline(file, line))
        if (!line.empty()) paths.push_back(line);
    return paths;
}

std::string decode(const std::string& text)
{
    const auto hex = [](char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    std::string out;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size() && hex(text[i + 1]) >= 0 && hex(text[i + 2]) >= 0) {
            out += static_cast<char>(hex(text[i + 1]) * 16 + hex(text[i + 2]));
            i += 2;
        } else if (text.compare(i, 5, "&amp;") == 0) {
            out += '&';
            i += 4;
        } else {
            out += text[i];
        }
    }
    return out;
}

} // namespace

namespace RecentFiles {

void add(const std::string& path)
{
    std::vector<std::string> paths = readOwn();
    paths.erase(std::remove(paths.begin(), paths.end(), path), paths.end());
    paths.insert(paths.begin(), path);
    if (paths.size() > kMaxItems) paths.resize(kMaxItems);

    std::error_code ignored;
    std::filesystem::create_directories(PathBar::parent(ownFile()), ignored);
    std::ofstream file(ownFile(), std::ios::trunc);
    for (const std::string& item : paths) file << item << '\n';
}

std::vector<std::string> parseXbel(const std::string& text)
{
    std::vector<std::string> paths;
    const std::string start = "href=\"file://";
    std::size_t at = 0;
    while ((at = text.find(start, at)) != std::string::npos) {
        at += start.size();
        const auto end = text.find('"', at);
        if (end == std::string::npos) break;
        paths.push_back(decode(text.substr(at, end - at)));
        at = end;
    }
    return paths;
}

std::vector<std::string> list()
{
    std::vector<std::string> paths = readOwn();

    std::ifstream xbel(dataHome() + "/recently-used.xbel");
    std::stringstream text;
    text << xbel.rdbuf();
    std::vector<std::string> others = parseXbel(text.str());
    std::reverse(others.begin(), others.end());            // the file lists the oldest first
    paths.insert(paths.end(), others.begin(), others.end());

    std::vector<std::string> present;
    for (const std::string& path : paths) {
        struct stat info {};
        if (stat(path.c_str(), &info) != 0 || !S_ISREG(info.st_mode)) continue;
        if (std::find(present.begin(), present.end(), path) != present.end()) continue;
        present.push_back(path);
        if (present.size() == kMaxItems) break;
    }
    return present;
}

void clear()
{
    std::error_code ignored;
    std::filesystem::remove(ownFile(), ignored);
}

} // namespace RecentFiles
