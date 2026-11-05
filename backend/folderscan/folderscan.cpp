#include "folderscan/folderscan.h"

#include <algorithm>
#include <chrono>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_set>

namespace {

constexpr std::size_t kLargestCount = 10;
constexpr auto kProgressEvery = std::chrono::milliseconds(120);

struct Walker {
    FolderScan& scan;
    const std::function<void(const FolderScan&)>& progress;
    const std::function<bool()>& cancelled;
    bool oneDisk;
    dev_t device;
    std::unordered_set<std::string> seenHardLinks;
    std::chrono::steady_clock::time_point lastReport = std::chrono::steady_clock::now();

    void addLargest(std::uint64_t size, const std::string& path)
    {
        auto& list = scan.largest;
        if (list.size() == kLargestCount && size <= list.back().first) return;
        const auto place = std::upper_bound(list.begin(), list.end(), size,
                                            [](std::uint64_t value, const auto& item) { return value > item.first; });
        list.insert(place, {size, path});
        if (list.size() > kLargestCount) list.pop_back();
    }

    // one item: counted, and walked into when it is a folder. False when cancelled.
    bool visit(int parentFd, const std::string& name, const std::string& full)
    {
        struct stat info {};
        if (fstatat(parentFd, name.c_str(), &info, AT_SYMLINK_NOFOLLOW) != 0) return true;
        if (oneDisk && info.st_dev != device) return true;
        const auto slash = full.rfind('/');
        if (full[slash == std::string::npos ? 0 : slash + 1] == '.') ++scan.hidden;

        if (S_ISDIR(info.st_mode)) {
            ++scan.folders;
            scan.diskUsage += static_cast<std::uint64_t>(info.st_blocks) * 512;
            const int child = openat(parentFd, name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (child < 0) ++scan.unreadable;
            else if (!walk(child, full)) return false;
        } else if (S_ISLNK(info.st_mode)) {
            ++scan.links;
        } else if (S_ISREG(info.st_mode)) {
            ++scan.files;
            scan.totalSize += static_cast<std::uint64_t>(info.st_size);
            addLargest(static_cast<std::uint64_t>(info.st_size), full);
            const bool firstSighting = info.st_nlink < 2
                || seenHardLinks.insert(std::to_string(info.st_dev) + ":" + std::to_string(info.st_ino)).second;
            if (firstSighting) scan.diskUsage += static_cast<std::uint64_t>(info.st_blocks) * 512;
        } else {
            ++scan.others;
        }

        const auto now = std::chrono::steady_clock::now();
        if (progress && now - lastReport >= kProgressEvery) {
            lastReport = now;
            progress(scan);
        }
        return true;
    }

    // takes ownership of fd
    bool walk(int fd, const std::string& path)
    {
        DIR* dir = fdopendir(fd);
        if (!dir) {
            close(fd);
            ++scan.unreadable;
            return true;
        }
        while (dirent* item = readdir(dir)) {
            if (cancelled && cancelled()) {
                closedir(dir);
                return false;
            }
            const std::string name = item->d_name;
            if (name == "." || name == "..") continue;
            if (!visit(dirfd(dir), name, path == "/" ? "/" + name : path + "/" + name)) {
                closedir(dir);
                return false;
            }
        }
        closedir(dir);
        return true;
    }
};

} // namespace

FolderScan scanFolder(const std::string& path, const std::function<void(const FolderScan&)>& progress,
                      const std::function<bool()>& cancelled, bool oneDisk)
{
    FolderScan scan;
    const int fd = open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) {
        scan.unreadable = 1;
        scan.finished = true;
        return scan;
    }
    struct stat info {};
    fstat(fd, &info);
    Walker walker {scan, progress, cancelled, oneDisk, info.st_dev, {}};
    scan.finished = walker.walk(fd, path);
    return scan;
}

FolderScan scanItems(const std::vector<std::string>& paths, const std::function<void(const FolderScan&)>& progress,
                     const std::function<bool()>& cancelled)
{
    FolderScan scan;
    Walker walker {scan, progress, cancelled, false, 0, {}};
    scan.finished = true;
    for (const std::string& path : paths) {
        if (cancelled && cancelled()) {
            scan.finished = false;
            break;
        }
        if (!walker.visit(AT_FDCWD, path, path)) {
            scan.finished = false;
            break;
        }
    }
    return scan;
}
