#include "directory/directorylister.h"

#include "filetype/filetype.h"

#include <atomic>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <mutex>
#include <thread>
#include <unistd.h>

namespace {

std::string errorSentence(int error)
{
    switch (error) {
    case EACCES:
    case EPERM: return "You don't have permission to open this folder.";
    case ENOENT: return "This folder doesn't exist.";
    case ENOTDIR: return "This is not a folder.";
    default: return std::string("This folder can't be opened: ") + std::strerror(error) + ".";
    }
}

EntryKind kindFromDirent(unsigned char type)
{
    switch (type) {
    case DT_DIR: return EntryKind::Directory;
    case DT_REG: return EntryKind::File;
    case DT_SOCK: return EntryKind::Socket;
    case DT_FIFO: return EntryKind::Pipe;
    case DT_CHR: return EntryKind::CharDevice;
    case DT_BLK: return EntryKind::BlockDevice;
    default: return EntryKind::Unknown;
    }
}

} // namespace

ListResult listDirectory(const std::string& path, const SortSettings& sort,
                         const std::function<bool()>& cancelled)
{
    ListResult result;
    result.path = path;
    result.sort = sort;

    const int fd = open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) {
        result.error = errorSentence(errno);
        return result;
    }
    DIR* dir = fdopendir(fd);
    if (!dir) {
        result.error = errorSentence(errno);
        close(fd);
        return result;
    }

    while (dirent* item = readdir(dir)) {
        if (cancelled && cancelled()) {
            closedir(dir);
            return result;
        }
        const std::string name = item->d_name;
        if (name == "." || name == "..") continue;

        Entry entry;
        if (!readEntry(dirfd(dir), path, name, entry)) {
            if (errno == ENOENT) continue;                 // it was deleted a moment ago
            // the name is readable but its details are not (a folder with r but no x): show what is known
            entry.name = name;
            entry.path = path == "/" ? "/" + name : path + "/" + name;
            entry.kind = kindFromDirent(item->d_type);
            entry.hidden = name[0] == '.';
        }
        detectFileType(entry);
        result.entries.push_back(std::move(entry));
    }
    closedir(dir);

    sortEntries(result.entries, sort);
    result.disk = readDiskSpace(path);
    result.ok = true;
    return result;
}

struct DirectoryLister::Job {
    std::atomic<bool> cancelled {false};
    std::mutex deliver;                    // held while the callback runs, so cancel() can wait for it
};

void DirectoryLister::start(const std::string& path, const SortSettings& sort, Callback callback)
{
    cancel();
    auto job = std::make_shared<Job>();
    m_job = job;
    std::thread([job, path, sort, callback = std::move(callback)] {
        ListResult result = listDirectory(path, sort, [&job] { return job->cancelled.load(); });
        std::lock_guard lock(job->deliver);
        if (!job->cancelled) callback(std::move(result));
    }).detach();
}

void DirectoryLister::cancel()
{
    if (!m_job) return;
    m_job->cancelled = true;
    std::lock_guard lock(m_job->deliver);  // a callback that already started finishes first
    m_job.reset();
}
