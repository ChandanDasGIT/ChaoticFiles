#include "transfer/transfer.h"

#include "fileops/fileops.h"
#include "pathbar/pathbar.h"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

namespace {

constexpr std::size_t kBufferSize = 1024 * 1024;
constexpr std::int64_t kReportEveryMs = 80;

std::string join(const std::string& folder, const std::string& name)
{
    return folder == "/" ? "/" + name : folder + "/" + name;
}

std::vector<std::string> namesIn(const std::string& path)
{
    std::vector<std::string> names;
    DIR* dir = opendir(path.c_str());
    if (!dir) return names;
    while (dirent* item = readdir(dir)) {
        const std::string name = item->d_name;
        if (name != "." && name != "..") names.push_back(name);
    }
    closedir(dir);
    return names;
}

void copyUserAttributes(int from, int to)
{
    const ssize_t size = flistxattr(from, nullptr, 0);
    if (size <= 0) return;
    std::string names(static_cast<std::size_t>(size), '\0');
    const ssize_t got = flistxattr(from, names.data(), names.size());
    if (got <= 0) return;

    for (std::size_t at = 0; at < static_cast<std::size_t>(got);) {
        const std::string name = names.c_str() + at;
        at += name.size() + 1;
        if (name.rfind("user.", 0) != 0) continue;
        const ssize_t length = fgetxattr(from, name.c_str(), nullptr, 0);
        if (length < 0) continue;
        std::string value(static_cast<std::size_t>(length), '\0');
        if (fgetxattr(from, name.c_str(), value.data(), value.size()) >= 0)
            fsetxattr(to, name.c_str(), value.data(), value.size(), 0);
    }
}

} // namespace

Transfer::Transfer(TransferKind kind, std::vector<std::string> sources, std::string destination, Events events)
    : m_kind(kind), m_sources(std::move(sources)), m_destination(std::move(destination)), m_events(std::move(events))
{
}

void Transfer::cancel()
{
    m_cancelled = true;
    std::lock_guard lock(m_pauseLock);
    m_paused = false;
    m_resume.notify_all();
}

void Transfer::setPaused(bool paused)
{
    std::lock_guard lock(m_pauseLock);
    m_paused = paused;
    m_resume.notify_all();
}

bool Transfer::shouldStop()
{
    std::unique_lock lock(m_pauseLock);
    m_resume.wait(lock, [this] { return !m_paused; });
    return m_cancelled;
}

void Transfer::report(const std::string& name, bool force)
{
    if (!m_events.progress) return;
    const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    if (!force && now - m_lastReport < kReportEveryMs) return;
    m_lastReport = now;
    m_progress.currentName = name;
    m_events.progress(m_progress);
}

void Transfer::fail(const std::string& path, int error)
{
    std::string sentence;
    switch (error) {
    case EACCES:
    case EPERM: sentence = "You don't have permission to use \"" + PathBar::baseName(path) + "\"."; break;
    case ENOSPC: sentence = "There is no space left on the disk."; break;
    case EROFS: sentence = "The destination is read-only."; break;
    case ENOENT: sentence = "\"" + PathBar::baseName(path) + "\" no longer exists."; break;
    default: sentence = "\"" + PathBar::baseName(path) + "\": " + std::strerror(error) + ".";
    }
    m_result.errors.push_back(sentence);
}

void Transfer::measure(const std::string& path, std::uint64_t& bytes, std::uint64_t& files) const
{
    struct stat info {};
    if (lstat(path.c_str(), &info) != 0) return;
    if (S_ISDIR(info.st_mode)) {
        for (const std::string& name : namesIn(path)) {
            if (m_cancelled) return;
            measure(join(path, name), bytes, files);
        }
        return;
    }
    ++files;
    if (S_ISREG(info.st_mode)) bytes += static_cast<std::uint64_t>(info.st_size);
}

TransferResult Transfer::run()
{
    struct stat destination {};
    if (stat(m_destination.c_str(), &destination) != 0 || !S_ISDIR(destination.st_mode)) {
        m_result.errors.push_back("The destination folder doesn't exist.");
        return m_result;
    }

    for (const std::string& source : m_sources) measure(source, m_progress.bytesTotal, m_progress.filesTotal);
    report("", true);

    for (const std::string& source : m_sources) {
        if (shouldStop()) break;
        const std::string target = join(m_destination, PathBar::baseName(source));
        if (m_destination == source || m_destination.rfind(source + "/", 0) == 0) {
            m_result.errors.push_back("\"" + PathBar::baseName(source) + "\" can't go inside itself.");
            continue;
        }
        processItem(source, target, true);
    }
    m_result.cancelled = m_cancelled;
    report("", true);
    return m_result;
}

Transfer::Decision Transfer::resolveConflict(const std::string& source, std::string& target)
{
    struct stat existing {};
    if (lstat(target.c_str(), &existing) != 0) return Decision::Write;

    // the same item: a copy gets a new name beside it, and a move has nothing to do
    if (source == target) {
        if (m_kind == TransferKind::Move) return Decision::Skip;
        target = FileOps::uniquePath(target);
        return Decision::Write;
    }

    ConflictChoice choice = ConflictChoice::Cancel;
    if (m_answerForAll) {
        choice = *m_answerForAll;
    } else if (m_events.conflict) {
        const ConflictAnswer answer = m_events.conflict(source, target);
        choice = answer.choice;
        if (answer.applyToAll) m_answerForAll = choice;
    }

    switch (choice) {
    case ConflictChoice::Overwrite: return Decision::WriteOverExisting;
    case ConflictChoice::Skip: return Decision::Skip;
    case ConflictChoice::KeepBoth:
        target = FileOps::uniquePath(target);
        return Decision::Write;
    case ConflictChoice::Cancel: break;
    }
    cancel();
    return Decision::Stop;
}

// returns true if everything of this item arrived, which is what allows a move to delete its source
bool Transfer::processItem(const std::string& source, const std::string& requestedTarget, bool topLevel)
{
    if (shouldStop()) return false;

    struct stat from {};
    if (lstat(source.c_str(), &from) != 0) {
        fail(source, errno);
        return false;
    }

    std::string target = requestedTarget;
    struct stat existing {};
    const bool exists = lstat(target.c_str(), &existing) == 0;

    // two folders with one name merge, whatever else happens
    if (exists && source != target && S_ISDIR(from.st_mode) && S_ISDIR(existing.st_mode)) {
        const bool complete = copyFolder(source, target);
        if (complete && m_kind == TransferKind::Move && rmdir(source.c_str()) != 0) fail(source, errno);
        if (complete && topLevel) ++m_result.merged;
        return complete;
    }

    const Decision decision = resolveConflict(source, target);
    if (decision == Decision::Stop) return false;
    if (decision == Decision::Skip) {
        std::uint64_t bytes = 0, files = 0;
        measure(source, bytes, files);
        m_progress.bytesDone += bytes;
        m_progress.filesDone += files;
        ++m_result.skipped;
        report(PathBar::baseName(source));
        return false;
    }

    const bool overwrite = decision == Decision::WriteOverExisting;
    if (overwrite && S_ISDIR(from.st_mode) != S_ISDIR(existing.st_mode)) {
        m_result.errors.push_back("\"" + PathBar::baseName(target) + "\" can't be replaced: one is a file and the other a folder.");
        return false;
    }

    bool complete = false;
    if (m_kind == TransferKind::Move) {
        // rename is instant and all or nothing; it only fails across disks or when nothing may be replaced
        std::uint64_t bytes = 0, files = 0;
        measure(source, bytes, files);
        const int status = overwrite ? ::rename(source.c_str(), target.c_str())
                                     : ::renameat2(AT_FDCWD, source.c_str(), AT_FDCWD, target.c_str(), RENAME_NOREPLACE);
        if (status == 0) {
            m_progress.bytesDone += bytes;
            m_progress.filesDone += files;
            report(PathBar::baseName(source));
            if (topLevel) m_result.done.emplace_back(source, target);
            return true;
        }
        if (errno != EXDEV && errno != EINVAL) {
            fail(source, errno);
            return false;
        }
        complete = copyItem(source, target, overwrite);
        if (complete) {
            const Outcome removed = FileOps::removeTree(source, {});
            if (!removed.ok) {
                m_result.errors.push_back(removed.error);
                complete = false;
            }
        }
    } else {
        complete = copyItem(source, target, overwrite);
    }
    if (complete && topLevel) m_result.done.emplace_back(source, target);
    return complete;
}

bool Transfer::copyItem(const std::string& source, const std::string& target, bool overwrite)
{
    struct stat from {};
    if (lstat(source.c_str(), &from) != 0) {
        fail(source, errno);
        return false;
    }

    if (S_ISDIR(from.st_mode)) return copyFolder(source, target);
    if (S_ISREG(from.st_mode)) return copyFile(source, target, overwrite);

    if (S_ISLNK(from.st_mode)) {
        std::string link(static_cast<std::size_t>(from.st_size) + 1, '\0');
        const ssize_t size = readlink(source.c_str(), link.data(), link.size());
        if (size < 0) {
            fail(source, errno);
            return false;
        }
        link.resize(static_cast<std::size_t>(size));
        if (overwrite) unlink(target.c_str());
        if (symlink(link.c_str(), target.c_str()) != 0) {
            fail(target, errno);
            return false;
        }
        ++m_progress.filesDone;
        report(PathBar::baseName(source));
        return true;
    }

    m_result.errors.push_back("\"" + PathBar::baseName(source) + "\" is a special file and can't be copied.");
    ++m_progress.filesDone;
    return false;
}

bool Transfer::copyFolder(const std::string& source, const std::string& target)
{
    struct stat from {};
    if (stat(source.c_str(), &from) != 0) {
        fail(source, errno);
        return false;
    }
    // the mode is applied at the end: a read-only folder must stay writable while it is being filled
    const bool created = mkdir(target.c_str(), 0700 | (from.st_mode & 0777)) == 0;
    if (!created && errno != EEXIST) {
        fail(target, errno);
        return false;
    }

    bool complete = true;
    for (const std::string& name : namesIn(source)) {
        if (shouldStop()) return false;
        complete = processItem(join(source, name), join(target, name), false) && complete;
    }
    if (created) {
        const int sourceFd = open(source.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        const int targetFd = open(target.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (sourceFd >= 0 && targetFd >= 0) copyUserAttributes(sourceFd, targetFd);
        if (sourceFd >= 0) close(sourceFd);
        if (targetFd >= 0) close(targetFd);
        chmod(target.c_str(), from.st_mode & 07777);
        const struct timespec times[2] = {from.st_atim, from.st_mtim};
        utimensat(AT_FDCWD, target.c_str(), times, 0);
    }
    return complete;
}

bool Transfer::copyData(int from, int to, std::uint64_t size, const std::string& name)
{
    // a reflink shares the data on btrfs and xfs: instant, and uses no space
    if (ioctl(to, FICLONE, from) == 0) {
        m_progress.bytesDone += size;
        return true;
    }

    std::uint64_t left = size;
    bool useRange = true;
    std::vector<char> buffer;
    while (left > 0) {
        if (shouldStop()) return false;
        ssize_t written = -1;
        if (useRange) {
            written = copy_file_range(from, nullptr, to, nullptr, std::min<std::uint64_t>(left, kBufferSize * 16), 0);
            if (written < 0 && (errno == EXDEV || errno == ENOSYS || errno == EINVAL || errno == EOPNOTSUPP)) {
                useRange = false;
                continue;
            }
            if (written == 0) return true;                  // the file got shorter while reading
        } else {
            if (buffer.empty()) buffer.resize(kBufferSize);
            const ssize_t got = read(from, buffer.data(), buffer.size());
            if (got < 0) return false;
            if (got == 0) return true;
            ssize_t sent = 0;
            while (sent < got) {
                const ssize_t now = write(to, buffer.data() + sent, static_cast<std::size_t>(got - sent));
                if (now < 0) return false;
                sent += now;
            }
            written = got;
        }
        if (written < 0) return false;
        left -= std::min<std::uint64_t>(left, static_cast<std::uint64_t>(written));
        m_progress.bytesDone += static_cast<std::uint64_t>(written);
        report(name);
    }
    return true;
}

bool Transfer::copyFile(const std::string& source, const std::string& target, bool overwrite)
{
    const std::string name = PathBar::baseName(source);
    report(name);

    const int from = open(source.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (from < 0) {
        fail(source, errno);
        return false;
    }
    struct stat info {};
    fstat(from, &info);

    const std::string temporary = PathBar::parent(target) + "/.chaoticfiles-part-" + std::to_string(getpid())
                                + "-" + std::to_string(info.st_ino);
    const int to = open(temporary.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, (info.st_mode & 0777) | 0200);
    if (to < 0) {
        fail(target, errno);
        close(from);
        return false;
    }

    const std::uint64_t doneBefore = m_progress.bytesDone;
    errno = 0;
    bool ok = copyData(from, to, static_cast<std::uint64_t>(info.st_size), name);
    int error = errno;
    if (ok) {
        copyUserAttributes(from, to);
        fchmod(to, info.st_mode & 07777);
        const struct timespec times[2] = {info.st_atim, info.st_mtim};
        futimens(to, times);
    }
    close(from);
    if (close(to) != 0 && ok) {              // a full disk can show up only here
        ok = false;
        error = errno;
    }

    if (ok) {
        const int status = overwrite ? ::rename(temporary.c_str(), target.c_str())
                                     : ::renameat2(AT_FDCWD, temporary.c_str(), AT_FDCWD, target.c_str(), RENAME_NOREPLACE);
        ok = status == 0;
        if (!ok && errno == EINVAL && !overwrite) {       // a filesystem without NOREPLACE
            struct stat present {};
            ok = lstat(target.c_str(), &present) != 0 && ::rename(temporary.c_str(), target.c_str()) == 0;
        }
        if (!ok) error = errno;
    }
    if (!ok) {
        unlink(temporary.c_str());
        m_progress.bytesDone = doneBefore;
        if (!m_cancelled) fail(target, error ? error : EIO);
        return false;
    }
    ++m_progress.filesDone;
    report(name);
    return true;
}
