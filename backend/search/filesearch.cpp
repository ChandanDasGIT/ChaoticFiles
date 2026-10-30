#include "search/filesearch.h"

#include "filetype/filetype.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

constexpr auto kBatchEvery = std::chrono::milliseconds(200);

char lower(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

struct Walker {
    std::string query;
    bool showHidden;
    std::size_t limit;
    const std::function<void(std::vector<Entry>)>& batch;
    const std::function<bool()>& cancelled;
    dev_t device;
    std::size_t found = 0;
    std::vector<Entry> pending;
    std::chrono::steady_clock::time_point lastBatch = std::chrono::steady_clock::now();

    void flush()
    {
        if (pending.empty()) return;
        batch(std::move(pending));
        pending.clear();
        lastBatch = std::chrono::steady_clock::now();
    }

    bool walk(const std::string& path)        // false means stop everything
    {
        const int fd = open(path.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        if (fd < 0) return true;
        DIR* dir = fdopendir(fd);
        if (!dir) {
            close(fd);
            return true;
        }

        std::vector<std::string> folders;
        while (dirent* item = readdir(dir)) {
            if (cancelled && cancelled()) {
                closedir(dir);
                return false;
            }
            const std::string name = item->d_name;
            if (name == "." || name == ".." || (!showHidden && name[0] == '.')) continue;

            if (FileSearch::nameMatches(name, query)) {
                Entry entry;
                if (readEntry(dirfd(dir), path, name, entry)) {
                    detectFileType(entry);
                    entry.location = path;
                    pending.push_back(std::move(entry));
                    if (++found >= limit) {
                        closedir(dir);
                        return false;
                    }
                }
            }
            struct stat info {};
            if (item->d_type == DT_DIR
                || (item->d_type == DT_UNKNOWN && fstatat(dirfd(dir), name.c_str(), &info, AT_SYMLINK_NOFOLLOW) == 0 && S_ISDIR(info.st_mode)))
                folders.push_back(path == "/" ? "/" + name : path + "/" + name);
        }
        closedir(dir);

        if (std::chrono::steady_clock::now() - lastBatch >= kBatchEvery) flush();
        for (const std::string& folder : folders) {
            struct stat info {};
            if (lstat(folder.c_str(), &info) != 0 || info.st_dev != device) continue;
            if (!walk(folder)) return false;
        }
        return true;
    }
};

} // namespace

namespace FileSearch {

bool nameMatches(const std::string& name, const std::string& query)
{
    return std::search(name.begin(), name.end(), query.begin(), query.end(),
                       [](char a, char b) { return lower(a) == lower(b); }) != name.end();
}

void run(const std::string& root, const std::string& query, bool showHidden, std::size_t limit,
         const std::function<void(std::vector<Entry>)>& batch, const std::function<bool()>& cancelled)
{
    struct stat info {};
    if (stat(root.c_str(), &info) != 0 || query.empty()) return;
    Walker walker {query, showHidden, limit, batch, cancelled, info.st_dev, 0, {}, std::chrono::steady_clock::now()};
    walker.walk(root);
    walker.flush();
}

} // namespace FileSearch
