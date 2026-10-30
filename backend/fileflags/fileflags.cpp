#include "fileflags/fileflags.h"

#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

struct FlagName {
    unsigned long flag;
    char letter;
    const char* name;
};

const FlagName kFlags[] = {
    {FS_SECRM_FL, 's', "secure delete"}, {FS_UNRM_FL, 'u', "undelete"}, {FS_SYNC_FL, 'S', "synchronous updates"},
    {FS_DIRSYNC_FL, 'D', "synchronous folder updates"}, {FS_IMMUTABLE_FL, 'i', "immutable"},
    {FS_APPEND_FL, 'a', "append only"}, {FS_NODUMP_FL, 'd', "no dump"}, {FS_NOATIME_FL, 'A', "no access time"},
    {FS_COMPR_FL, 'c', "compressed"}, {FS_ENCRYPT_FL, 'E', "encrypted"}, {FS_JOURNAL_DATA_FL, 'j', "journalled data"},
    {FS_INDEX_FL, 'I', "indexed folder"}, {FS_NOTAIL_FL, 't', "no tail merging"}, {FS_TOPDIR_FL, 'T', "top of folder tree"},
    {FS_EXTENT_FL, 'e', "extents"}, {FS_VERITY_FL, 'V', "verity"}, {FS_NOCOW_FL, 'C', "no copy on write"},
    {FS_DAX_FL, 'x', "direct access"}, {FS_CASEFOLD_FL, 'F', "case insensitive"}, {FS_PROJINHERIT_FL, 'P', "project inherit"},
};

} // namespace

namespace FileFlags {

Flags read(const std::string& path)
{
    Flags result;
    struct stat info {};
    if (lstat(path.c_str(), &info) != 0 || !(S_ISREG(info.st_mode) || S_ISDIR(info.st_mode))) return result;

    const int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return result;
    long flags = 0;
    const int status = ioctl(fd, FS_IOC_GETFLAGS, &flags);
    close(fd);
    if (status != 0) return result;

    result.known = true;
    for (const FlagName& entry : kFlags) {
        const bool on = (static_cast<unsigned long>(flags) & entry.flag) != 0;
        result.letters += on ? entry.letter : '-';
        if (on) result.names += (result.names.empty() ? "" : ", ") + std::string(entry.name);
    }
    return result;
}

} // namespace FileFlags
