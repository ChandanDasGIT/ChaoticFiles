#include "diskusage/diskusage.h"

#include "folderscan/folderscan.h"

#include <algorithm>
#include <chrono>
#include <dirent.h>
#include <sys/stat.h>

namespace {

constexpr auto kProgressEvery = std::chrono::milliseconds(150);

void add(UsageReport& report, UsageItem item)
{
    report.total += item.bytes;
    auto& items = report.items;
    const auto place = std::upper_bound(items.begin(), items.end(), item.bytes,
                                        [](std::uint64_t bytes, const UsageItem& other) { return bytes > other.bytes; });
    items.insert(place, std::move(item));
    if (items.size() > kMaxUsageItems) {
        items.pop_back();
        ++report.hiddenItems;
    }
}

} // namespace

UsageReport scanUsage(const std::string& path, const std::function<void(const UsageReport&)>& progress,
                      const std::function<bool()>& cancelled)
{
    UsageReport report;
    struct stat folder {};
    DIR* dir = stat(path.c_str(), &folder) == 0 ? opendir(path.c_str()) : nullptr;
    if (!dir) {
        report.unreadable = 1;
        report.finished = true;
        return report;
    }

    std::vector<std::string> names;
    while (dirent* item = readdir(dir)) {
        const std::string name = item->d_name;
        if (name != "." && name != "..") names.push_back(name);
    }
    closedir(dir);
    report.count = names.size();

    auto lastReport = std::chrono::steady_clock::now() - kProgressEvery;
    for (const std::string& name : names) {
        if (cancelled && cancelled()) return report;

        const std::string full = path == "/" ? "/" + name : path + "/" + name;
        struct stat info {};
        if (lstat(full.c_str(), &info) == 0 && info.st_dev == folder.st_dev) {
            UsageItem item;
            item.name = name;
            item.path = full;
            item.folder = S_ISDIR(info.st_mode);
            item.bytes = static_cast<std::uint64_t>(info.st_blocks) * 512;
            if (item.folder) {
                report.current = name;
                if (progress) progress(report);
                const FolderScan scan = scanFolder(full, {}, cancelled, true);
                if (!scan.finished) return report;
                item.bytes += scan.diskUsage;
                item.files = scan.files;
                report.unreadable += scan.unreadable;
                lastReport = std::chrono::steady_clock::now() - kProgressEvery;
            } else {
                item.files = S_ISREG(info.st_mode) ? 1 : 0;
            }
            add(report, std::move(item));
        }
        ++report.done;
        const auto now = std::chrono::steady_clock::now();
        if (progress && now - lastReport >= kProgressEvery) {
            lastReport = now;
            progress(report);
        }
    }
    report.current.clear();
    report.finished = true;
    return report;
}
