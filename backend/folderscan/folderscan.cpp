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

            struct stat info {};
            if (fstatat(dirfd(dir), name.c_str(), &info, AT_SYMLINK_NOFOLLOW) != 0) continue;
            if (oneDisk && info.st_dev != device) continue;
            const std::string full = path == "/" ? "/" + name : path + "/" + name;
            if (name[0] == '.') ++scan.hidden;

            if (S_ISDIR(info.st_mode)) {
                ++scan.folders;
                scan.diskUsage += static_cast<std::uint64_t>(info.st_blocks) * 512;
                const int child = openat(dirfd(dir), name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
                if (child < 0) ++scan.unreadable;
                else if (!walk(child, full)) {
                    closedir(dir);
                    return false;
                }
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
