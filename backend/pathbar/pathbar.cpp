#include "pathbar/pathbar.h"

#include "sorting/sorting.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <dirent.h>
#include <filesystem>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

std::string percentDecode(const std::string& text)
{
    const auto hex = [](char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size() && hex(text[i + 1]) >= 0 && hex(text[i + 2]) >= 0) {
            out += static_cast<char>(hex(text[i + 1]) * 16 + hex(text[i + 2]));
            i += 2;
        } else {
            out += text[i];
        }
    }
    return out;
}

std::string trimmed(const std::string& text)
{
    const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    const auto begin = std::find_if_not(text.begin(), text.end(), isSpace);
    const auto end = std::find_if_not(text.rbegin(), std::make_reverse_iterator(begin), isSpace).base();
    return std::string(begin, end);
}

bool startsWithIgnoringCase(const std::string& text, const std::string& prefix)
{
    return text.size() >= prefix.size()
        && std::equal(prefix.begin(), prefix.end(), text.begin(), [](unsigned char a, unsigned char b) {
               return std::tolower(a) == std::tolower(b);
           });
}

} // namespace

namespace PathBar {

std::string homeDirectory()
{
    if (const char* home = std::getenv("HOME"); home && *home) return normalize(home);
    if (const passwd* user = getpwuid(getuid()); user && user->pw_dir) return normalize(user->pw_dir);
    return "/";
}

std::string normalize(const std::string& path)
{
    std::string out = std::filesystem::path(path).lexically_normal().string();
    while (out.size() > 1 && out.back() == '/') out.pop_back();
    return out.empty() ? "/" : out;
}

std::string parent(const std::string& path)
{
    const auto slash = path.rfind('/');
    if (slash == std::string::npos || slash == 0) return "/";
    return path.substr(0, slash);
}

std::string baseName(const std::string& path)
{
    const auto slash = path.rfind('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

std::string tabTitle(const std::string& path)
{
    if (path == "/") return "/";
    if (path == homeDirectory()) return "Home";
    return baseName(path);
}

bool isDirectory(const std::string& path)
{
    struct stat info {};
    return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

std::vector<PathPart> split(const std::string& path)
{
    std::vector<PathPart> parts {{"/", "/"}};
    std::size_t start = 1;
    while (start < path.size()) {
        const auto slash = path.find('/', start);
        const auto end = slash == std::string::npos ? path.size() : slash;
        parts.push_back({path.substr(start, end - start), path.substr(0, end)});
        start = end + 1;
    }
    return parts;
}

std::vector<PathPart> subfolders(const std::string& path, bool showHidden)
{
    std::vector<PathPart> folders;
    DIR* dir = opendir(path.c_str());
    if (!dir) return folders;

    while (dirent* item = readdir(dir)) {
        const std::string name = item->d_name;
        if (name == "." || name == ".." || (!showHidden && name[0] == '.')) continue;

        const std::string full = path == "/" ? "/" + name : path + "/" + name;
        const bool folder = item->d_type == DT_DIR
            || ((item->d_type == DT_LNK || item->d_type == DT_UNKNOWN) && isDirectory(full));
        if (folder) folders.push_back({name, full});
    }
    closedir(dir);

    std::sort(folders.begin(), folders.end(), [](const PathPart& a, const PathPart& b) {
        return naturalCompare(a.label, b.label) < 0;
    });
    return folders;
}

std::string resolve(const std::string& typed, const std::string& currentFolder)
{
    std::string text = trimmed(typed);
    if (text.rfind("file://", 0) == 0) text = percentDecode(text.substr(7));

    if (text.empty()) return currentFolder;
    if (text == "~") return homeDirectory();
    if (text.rfind("~/", 0) == 0) return normalize(homeDirectory() + text.substr(1));
    if (text[0] == '/') return normalize(text);
    return normalize(currentFolder + "/" + text);
}

std::vector<PathPart> suggestions(const std::string& typed, const std::string& currentFolder,
                                  bool showHidden, std::size_t limit)
{
    if (trimmed(typed).empty()) return {};

    const bool wantsInside = typed.back() == '/';
    const std::string resolved = resolve(typed, currentFolder);
    const std::string folder = wantsInside ? resolved : parent(resolved);
    const std::string prefix = wantsInside ? std::string() : baseName(resolved);

    std::vector<PathPart> matches;
    for (PathPart& part : subfolders(folder, showHidden || (!prefix.empty() && prefix[0] == '.'))) {
        if (!startsWithIgnoringCase(part.label, prefix)) continue;
        matches.push_back(std::move(part));
        if (matches.size() == limit) break;
    }
    return matches;
}

} // namespace PathBar
