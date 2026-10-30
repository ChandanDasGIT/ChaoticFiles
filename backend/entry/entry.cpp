#include "entry/entry.h"

#include <fcntl.h>
#include <sys/stat.h>

namespace {

EntryKind kindOf(std::uint16_t mode)
{
    if (S_ISDIR(mode)) return EntryKind::Directory;
    if (S_ISREG(mode)) return EntryKind::File;
    if (S_ISSOCK(mode)) return EntryKind::Socket;
    if (S_ISFIFO(mode)) return EntryKind::Pipe;
    if (S_ISCHR(mode)) return EntryKind::CharDevice;
    if (S_ISBLK(mode)) return EntryKind::BlockDevice;
    return EntryKind::Unknown;
}

} // namespace

bool readEntry(int dirFd, const std::string& dirPath, const std::string& name, Entry& out)
{
    const unsigned mask = STATX_TYPE | STATX_MODE | STATX_SIZE | STATX_MTIME;
    struct statx st {};
    if (statx(dirFd, name.c_str(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &st) != 0)
        return false;

    out.name = name;
    out.path = dirPath == "/" ? "/" + name : dirPath + "/" + name;
    out.hidden = !name.empty() && name[0] == '.';
    out.symlink = S_ISLNK(st.stx_mode);

    if (out.symlink) {
        struct statx target {};
        if (statx(dirFd, name.c_str(), AT_NO_AUTOMOUNT, mask, &target) == 0) {
            out.kind = kindOf(target.stx_mode);
            out.size = target.stx_size;
        } else {
            out.brokenLink = true;
            out.size = st.stx_size;
        }
    } else {
        out.kind = kindOf(st.stx_mode);
        out.size = st.stx_size;
    }

    out.mode = st.stx_mode & 07777;
    out.modified = st.stx_mtime.tv_sec;
    out.modifiedNanos = st.stx_mtime.tv_nsec;
    return true;
}

bool sameOnDisk(const Entry& a, const Entry& b)
{
    return a.name == b.name && a.size == b.size && a.modified == b.modified
        && a.modifiedNanos == b.modifiedNanos && a.kind == b.kind && a.mode == b.mode
        && a.symlink == b.symlink && a.brokenLink == b.brokenLink;
}
