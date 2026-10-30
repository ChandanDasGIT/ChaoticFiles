#include "diskspace/diskspace.h"

#include <sys/statvfs.h>

DiskSpace readDiskSpace(const std::string& path)
{
    DiskSpace space;
    struct statvfs info {};
    if (statvfs(path.c_str(), &info) != 0) return space;
    space.total = static_cast<std::uint64_t>(info.f_blocks) * info.f_frsize;
    space.available = static_cast<std::uint64_t>(info.f_bavail) * info.f_frsize;
    space.known = true;
    return space;
}
