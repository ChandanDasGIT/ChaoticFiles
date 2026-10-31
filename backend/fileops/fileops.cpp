#include "fileops/fileops.h"

#include "pathbar/pathbar.h"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

std::string errorSentence(const std::string& name, int error)
{
    switch (error) {
    case EACCES:
    case EPERM: return "You don't have permission to change \"" + name + "\".";
    case EEXIST:
    case ENOTEMPTY: return "\"" + name + "\" already exists.";
    case ENOENT: return "\"" + name + "\" no longer exists.";
    case ENOSPC: return "There is no space left on the disk.";
    case EROFS: return "This disk is read-only.";
    case EXDEV: return "\"" + name + "\" is on a different disk.";
    case EBUSY: return "\"" + name + "\" is in use.";
    case ENAMETOOLONG: return "The name is too long.";
    default: return "\"" + name + "\" failed: " + std::strerror(error) + ".";
    }
}

Outcome failure(const std::string& path, int error)
{
    return {false, errorSentence(PathBar::baseName(path), error)};
}

// takes ownership of fd; deletes everything inside it
Outcome removeContents(int fd, dev_t device, const std::string& path, const std::function<bool()>& cancelled)
{
    DIR* dir = fdopendir(fd);
    if (!dir) {
        close(fd);
        return failure(path, errno);
    }
    Outcome outcome;
    while (dirent* item = readdir(dir)) {
        if (cancelled && cancelled()) {
            outcome = {false, "Cancelled."};
            break;
        }
        const std::string name = item->d_name;
        if (name == "." || name == "..") continue;

        struct stat info {};
        if (fstatat(dirfd(dir), name.c_str(), &info, AT_SYMLINK_NOFOLLOW) != 0) {
            if (errno == ENOENT) continue;
            outcome = failure(name, errno);
            break;
        }
        if (S_ISDIR(info.st_mode)) {
            if (info.st_dev != device) {
                outcome = {false, "\"" + name + "\" is another mounted disk, so it was left alone."};
                break;
            }
            const int child = openat(dirfd(dir), name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
            if (child < 0) {
                outcome = failure(name, errno);
                break;
            }
            outcome = removeContents(child, device, path + "/" + name, cancelled);
            if (!outcome.ok) break;
            if (unlinkat(dirfd(dir), name.c_str(), AT_REMOVEDIR) != 0) {
                outcome = failure(name, errno);
                break;
            }
        } else if (unlinkat(dirfd(dir), name.c_str(), 0) != 0) {
            outcome = failure(name, errno);
            break;
        }
    }
    closedir(dir);
    return outcome;
}

} // namespace

namespace FileOps {

Outcome createFolder(const std::string& path)
{
    if (mkdir(path.c_str(), 0777) != 0) return failure(path, errno);
    return {};
}

Outcome createFile(const std::string& path)
{
    const int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0666);
    if (fd < 0) return failure(path, errno);
    close(fd);
    return {};
}

Outcome createLink(const std::string& target, const std::string& linkPath)
{
    if (symlink(target.c_str(), linkPath.c_str()) != 0) return failure(linkPath, errno);
    return {};
}

Outcome rename(const std::string& from, const std::string& to)
{
    if (from == to) return {};
    // renameat2 with NOREPLACE: a plain rename would silently overwrite
    if (::renameat2(AT_FDCWD, from.c_str(), AT_FDCWD, to.c_str(), RENAME_NOREPLACE) != 0) {
        if (errno != EINVAL) return failure(errno == EEXIST ? to : from, errno);
        struct stat info {};                  // a filesystem without NOREPLACE
        if (lstat(to.c_str(), &info) == 0) return failure(to, EEXIST);
        if (::rename(from.c_str(), to.c_str()) != 0) return failure(from, errno);
    }
    return {};
}

bool alreadyIn(const std::string& path, const std::string& folder)
{
    return path == folder || PathBar::parent(path) == folder;
}

std::string uniquePath(const std::string& path)
{
    struct stat info {};
    if (lstat(path.c_str(), &info) != 0) return path;

    const std::string folder = PathBar::parent(path), name = PathBar::baseName(path);
    auto dot = name.find('.', 1);                       // "a.tar.gz" keeps ".tar.gz" together
    if (dot == std::string::npos) dot = name.size();
    const std::string stem = name.substr(0, dot), extension = name.substr(dot);
    const std::string prefix = folder == "/" ? "/" : folder + "/";

    for (int number = 2;; ++number) {
        const std::string candidate = prefix + stem + " (" + std::to_string(number) + ")" + extension;
        if (lstat(candidate.c_str(), &info) != 0) return candidate;
    }
}

std::string nameProblem(const std::string& name)
{
    if (name.empty()) return "The name can't be empty.";
    if (name == "." || name == "..") return "This name is reserved.";
    if (name.find('/') != std::string::npos) return "A name can't contain \"/\".";
    if (name.find('\0') != std::string::npos) return "This name has an invalid character.";
    if (name.size() > 255) return "The name is too long.";
    return {};
}

bool isProtected(const std::string& path)
{
    const std::string clean = PathBar::normalize(path);
    if (clean == "/" || PathBar::parent(clean) == "/") return true;
    const std::string home = PathBar::homeDirectory();
    return clean == home || home.rfind(clean + "/", 0) == 0;
}

Outcome removeTree(const std::string& path, const std::function<bool()>& cancelled)
{
    if (isProtected(path)) return {false, "\"" + path + "\" is a system folder and can't be deleted."};

    struct stat info {};
    if (lstat(path.c_str(), &info) != 0) return failure(path, errno);
    if (!S_ISDIR(info.st_mode)) {
        if (unlink(path.c_str()) != 0) return failure(path, errno);
        return {};
    }

    const int fd = open(path.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return failure(path, errno);
    Outcome outcome = removeContents(fd, info.st_dev, path, cancelled);
    if (!outcome.ok) return outcome;
    if (rmdir(path.c_str()) != 0) return failure(path, errno);
    return {};
}

bool onSameDevice(const std::string& a, const std::string& b)
{
    struct stat first {}, second {};
    return stat(a.c_str(), &first) == 0 && stat(b.c_str(), &second) == 0 && first.st_dev == second.st_dev;
}

} // namespace FileOps
