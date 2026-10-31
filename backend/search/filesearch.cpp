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
constexpr off_t kMaxFileSize = 32 * 1024 * 1024;     // bigger text files are logs and dumps, and memory is better kept
constexpr std::size_t kBinaryCheck = 8192;
constexpr std::size_t kMaxLineShown = 160;

char lower(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

// Reads a whole file, unless it is big or cancelled. False means there is nothing to look at.
bool readFile(int dirFd, const std::string& name, std::string& text, const std::function<bool()>& cancelled)
{
    const int fd = openat(dirFd, name.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) return false;
    struct stat info {};
    if (fstat(fd, &info) != 0 || !S_ISREG(info.st_mode) || info.st_size == 0 || info.st_size > kMaxFileSize) {
        close(fd);
        return false;
    }
    text.resize(static_cast<std::size_t>(info.st_size));
    std::size_t done = 0;
    while (done < text.size()) {
        if (cancelled && cancelled()) break;
        const ssize_t got = read(fd, text.data() + done, std::min<std::size_t>(text.size() - done, 1024 * 1024));
        if (got <= 0) break;
        done += static_cast<std::size_t>(got);
    }
    close(fd);
    text.resize(done);
    return done > 0;
}

struct Walker {
    std::string query;
    bool inside;
    std::string root;
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

            std::string where = path;
            bool matches = !inside && FileSearch::nameMatches(name, query);
            if (inside && (item->d_type == DT_REG || item->d_type == DT_UNKNOWN)) {
                std::string text;
                const std::string line = readFile(dirfd(dir), name, text, cancelled) ? FileSearch::findInside(text, query) : std::string();
                matches = !line.empty();
                const std::string below = path.size() > root.size() ? path.substr(root.size() + (root == "/" ? 0 : 1)) : std::string();
                where = below.empty() ? line : below + ", " + line;
            }
            if (matches) {
                Entry entry;
                if (readEntry(dirfd(dir), path, name, entry)) {
                    detectFileType(entry);
                    entry.location = where;
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
    Walker walker {query, false, root, showHidden, limit, batch, cancelled, info.st_dev, 0, {}, std::chrono::steady_clock::now()};
    walker.walk(root);
    walker.flush();
}

std::string findInside(const std::string& text, const std::string& query)
{
    if (query.empty() || text.find('\0') < kBinaryCheck) return {};
    const auto found = std::search(text.begin(), text.end(), query.begin(), query.end(),
                                   [](char a, char b) { return lower(a) == lower(b); });
    if (found == text.end()) return {};

    const std::size_t at = static_cast<std::size_t>(found - text.begin());
    const std::size_t number = static_cast<std::size_t>(std::count(text.begin(), found, '\n')) + 1;
    const std::size_t before = at == 0 ? std::string::npos : text.rfind('\n', at - 1);
    std::size_t start = before == std::string::npos ? 0 : before + 1;
    std::size_t end = text.find('\n', at);
    if (end == std::string::npos) end = text.size();
    while (start < at && (text[start] == ' ' || text[start] == '\t')) ++start;
    while (end > at + query.size() && (text[end - 1] == '\r' || text[end - 1] == ' ' || text[end - 1] == '\t')) --end;

    // a long line shows the part with the match, cut between two letters and never inside one
    if (end - start > kMaxLineShown) {
        if (at - start > kMaxLineShown / 2) start = at - kMaxLineShown / 2;
        end = std::min(end, start + kMaxLineShown);
        while (start < at && (static_cast<unsigned char>(text[start]) & 0xC0) == 0x80) ++start;
        while (end < text.size() && end > start && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80) --end;
    }
    return "line " + std::to_string(number) + ": " + text.substr(start, end - start);
}

void runInside(const std::string& root, const std::string& query, bool showHidden, std::size_t limit,
               const std::function<void(std::vector<Entry>)>& batch, const std::function<bool()>& cancelled)
{
    struct stat info {};
    if (stat(root.c_str(), &info) != 0 || query.empty()) return;
    Walker walker {query, true, root, showHidden, limit, batch, cancelled, info.st_dev, 0, {}, std::chrono::steady_clock::now()};
    walker.walk(root);
    walker.flush();
}

} // namespace FileSearch
