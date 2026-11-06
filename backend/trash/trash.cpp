#include "trash/trash.h"

#include "mounts/mounts.h"
#include "pathbar/pathbar.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>

namespace {

std::string trashDirectory()
{
    const char* data = std::getenv("XDG_DATA_HOME");
    const std::string base = data && *data ? data : PathBar::homeDirectory() + "/.local/share";
    return PathBar::normalize(base + "/Trash");
}

bool makePrivate(const std::string& path)
{
    return mkdir(path.c_str(), 0700) == 0 || errno == EEXIST;
}

// lstat, so a link to a folder does not count
bool isOwnRealFolder(const std::string& path)
{
    struct stat info {};
    return lstat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode) && info.st_uid == getuid();
}

bool makeTrash(const std::string& trash)
{
    if (!makePrivate(trash) || !makePrivate(trash + "/files") || !makePrivate(trash + "/info")) return false;
    if (trash == trashDirectory() || Trash::isTrustworthy(trash)) return true;   // only this user writes in Home
    errno = EACCES;                                     // someone else's, or a link: never put anything there
    return false;
}

// Paths in a .trashinfo file are written like URLs: "My File" becomes "My%20File".
std::string encodePath(const std::string& path)
{
    static const char digits[] = "0123456789ABCDEF";
    std::string out;
    for (const unsigned char c : path) {
        const bool plain = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
                        || c == '/' || c == '-' || c == '_' || c == '.' || c == '~';
        if (plain) out += static_cast<char>(c);
        else {
            out += '%';
            out += digits[c >> 4];
            out += digits[c & 15];
        }
    }
    return out;
}

std::string decodePath(const std::string& text)
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
        } else {
            out += text[i];
        }
    }
    return out;
}

std::uint64_t mountIdOf(const std::string& path)
{
    struct statx info {};
    if (statx(AT_FDCWD, path.c_str(), AT_SYMLINK_NOFOLLOW, STATX_MNT_ID, &info) != 0) return 0;
    return info.stx_mnt_id;
}

std::string userTrashName()
{
    return ".Trash-" + std::to_string(getuid());
}

// every Trash of this user that exists: the one in Home, then the ones at the top of other disks
std::vector<std::string> allTrashes()
{
    std::vector<std::string> trashes {trashDirectory()};
    for (const Mount& mount : Mounts::all()) {
        const std::string top = mount.mountPoint == "/" ? "" : mount.mountPoint;
        const std::string trash = top + "/" + userTrashName();
        if (Trash::isTrustworthy(trash) && std::find(trashes.begin(), trashes.end(), trash) == trashes.end())
            trashes.push_back(trash);
    }
    return trashes;
}

struct InfoFile {
    std::string originalPath;
    std::int64_t deletedAt = 0;
};

// `top` is the disk's top folder for a Trash there: its paths are relative to it
InfoFile readInfo(const std::string& infoPath, const std::string& top)
{
    InfoFile info;
    std::ifstream file(infoPath);
    std::string line;
    while (std::getline(file, line)) {
        if (line.rfind("Path=", 0) == 0) {
            info.originalPath = decodePath(line.substr(5));
            if (!info.originalPath.empty() && info.originalPath[0] != '/') info.originalPath = top + "/" + info.originalPath;
        } else if (line.rfind("DeletionDate=", 0) == 0) {
            std::tm when {};
            when.tm_isdst = -1;
            if (strptime(line.c_str() + 13, "%Y-%m-%dT%H:%M:%S", &when)) info.deletedAt = std::mktime(&when);
        }
    }
    return info;
}

std::string trashOf(const std::string& trashedPath)         // ".../Trash/files/name" gives ".../Trash"
{
    return PathBar::parent(PathBar::parent(trashedPath));
}

std::string topOfTrash(const std::string& trash)
{
    return trash == trashDirectory() ? std::string() : PathBar::parent(trash);
}

// Puts one item into one Trash. EXDEV means this Trash is on another filesystem and the caller can try another.
int trashInto(const std::string& trash, const std::string& path, const std::string& recordedPath, std::string* trashedPath)
{
    if (!makeTrash(trash)) return errno ? errno : EACCES;

    // the .trashinfo file is created first and exclusively: that claims the name
    const std::string base = PathBar::baseName(path);
    std::string name, infoPath;
    for (int number = 1;; ++number) {
        name = number == 1 ? base : base + "." + std::to_string(number);
        infoPath = trash + "/info/" + name + ".trashinfo";
        const int fd = open(infoPath.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
        if (fd >= 0) {
            char when[32];
            const std::time_t now = std::time(nullptr);
            std::tm local {};
            localtime_r(&now, &local);
            std::strftime(when, sizeof when, "%Y-%m-%dT%H:%M:%S", &local);
            const std::string text = "[Trash Info]\nPath=" + encodePath(recordedPath) + "\nDeletionDate=" + when + "\n";
            const bool written = write(fd, text.data(), text.size()) == static_cast<ssize_t>(text.size());
            close(fd);
            if (!written) {
                unlink(infoPath.c_str());
                return ENOSPC;
            }
            break;
        }
        if (errno != EEXIST) return errno;
    }

    const std::string target = trash + "/files/" + name;
    if (::renameat2(AT_FDCWD, path.c_str(), AT_FDCWD, target.c_str(), RENAME_NOREPLACE) != 0) {
        const int error = errno;
        unlink(infoPath.c_str());
        return error;
    }
    if (trashedPath) *trashedPath = target;
    return 0;
}

} // namespace

namespace Trash {

std::string filesDirectory()
{
    return trashDirectory() + "/files";
}

bool ensureExists()
{
    const std::string trash = trashDirectory();
    std::error_code error;
    std::filesystem::create_directories(PathBar::parent(trash), error);
    return !error && makeTrash(trash);
}

bool isTrustworthy(const std::string& trash)
{
    return isOwnRealFolder(trash) && isOwnRealFolder(trash + "/files") && isOwnRealFolder(trash + "/info");
}

bool contains(const std::string& path)
{
    for (const std::string& trash : allTrashes())
        if (path.rfind(trash + "/", 0) == 0 || path == trash) return true;
    return false;
}

Outcome moveToTrash(const std::string& path, std::string* trashedPath)
{
    const std::string name = PathBar::baseName(path);
    if (FileOps::isProtected(path)) return {false, "\"" + path + "\" is a system folder and can't be moved to the Trash."};
    if (contains(path)) return {false, "\"" + name + "\" is already in the Trash."};

    ensureExists();
    int error = trashInto(trashDirectory(), path, path, trashedPath);
    if (error == 0) return {};

    if (error == EXDEV) {                     // another disk: use the Trash at its top
        const Mount mount = Mounts::find(mountIdOf(PathBar::parent(path)));
        if (!mount.mountPoint.empty() && mount.mountPoint != "/") {
            const std::string prefix = mount.mountPoint + "/";
            if (path.rfind(prefix, 0) == 0) {
                error = trashInto(mount.mountPoint + "/" + userTrashName(), path, path.substr(prefix.size()), trashedPath);
                if (error == 0) return {};
            }
        }
    }
    if (error == EACCES || error == EPERM) return {false, "You don't have permission to move \"" + name + "\" to the Trash."};
    if (error == EXDEV) return {false, "\"" + name + "\" is on a disk that has no Trash."};
    return {false, "\"" + name + "\" can't be moved to the Trash: " + std::strerror(error) + "."};
}

Outcome restore(const std::string& trashedPath, std::string* restoredTo)
{
    const std::string trash = trashOf(trashedPath), name = PathBar::baseName(trashedPath);
    const std::string infoPath = trash + "/info/" + name + ".trashinfo";
    const InfoFile info = readInfo(infoPath, topOfTrash(trash));
    if (info.originalPath.empty()) return {false, "The Trash has no record of where \"" + name + "\" came from."};

    std::error_code ignored;
    std::filesystem::create_directories(PathBar::parent(info.originalPath), ignored);
    Outcome moved = FileOps::rename(trashedPath, info.originalPath);
    if (!moved.ok) return moved;
    unlink(infoPath.c_str());
    if (restoredTo) *restoredTo = info.originalPath;
    return {};
}

Outcome remove(const std::string& trashedPath)
{
    if (!contains(trashedPath)) return {false, "This item is not in the Trash."};
    const Outcome removed = FileOps::removeTree(trashedPath, {});
    if (removed.ok) unlink((trashOf(trashedPath) + "/info/" + PathBar::baseName(trashedPath) + ".trashinfo").c_str());
    return removed;
}

std::vector<TrashItem> list()
{
    std::vector<TrashItem> items;
    for (const std::string& trash : allTrashes()) {
        const std::string files = trash + "/files";
        DIR* dir = opendir(files.c_str());
        if (!dir) continue;
        while (dirent* item = readdir(dir)) {
            const std::string name = item->d_name;
            if (name == "." || name == "..") continue;
            const InfoFile info = readInfo(trash + "/info/" + name + ".trashinfo", topOfTrash(trash));
            items.push_back({PathBar::baseName(info.originalPath.empty() ? name : info.originalPath),
                             files + "/" + name, info.originalPath, info.deletedAt});
        }
        closedir(dir);
    }
    return items;
}

Outcome empty()
{
    Outcome result;
    for (const TrashItem& item : list()) {
        const Outcome removed = remove(item.path);
        if (!removed.ok) result = removed;
    }
    return result;
}

} // namespace Trash
