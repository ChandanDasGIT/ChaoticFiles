#include "btrfs/btrfsinfo.h"

#include "xattrs/xattrs.h"

#include <cstring>
#include <fcntl.h>
#include <linux/btrfs.h>
#include <linux/fiemap.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {

constexpr std::uint64_t kRootDirectoryObject = 256;     // the root folder of any subvolume

void readExtents(int fd, BtrfsInfo& info)
{
    constexpr std::uint32_t kBatch = 256;
    std::vector<char> buffer(sizeof(fiemap) + kBatch * sizeof(fiemap_extent));
    auto* map = reinterpret_cast<fiemap*>(buffer.data());
    std::uint64_t start = 0;

    while (true) {
        std::memset(buffer.data(), 0, buffer.size());
        map->fm_start = start;
        map->fm_length = ~0ULL;
        map->fm_extent_count = kBatch;
        if (ioctl(fd, FS_IOC_FIEMAP, map) != 0 || map->fm_mapped_extents == 0) return;

        bool last = false;
        for (std::uint32_t i = 0; i < map->fm_mapped_extents; ++i) {
            const fiemap_extent& extent = map->fm_extents[i];
            ++info.extents;
            info.logicalBytes += extent.fe_length;
            if (extent.fe_flags & FIEMAP_EXTENT_ENCODED) {
                ++info.compressedExtents;
                info.compressedLogicalBytes += extent.fe_length;
            }
            if (extent.fe_flags & FIEMAP_EXTENT_SHARED) info.sharedBytes += extent.fe_length;
            else info.exclusiveBytes += extent.fe_length;
            start = extent.fe_logical + extent.fe_length;
            last = last || (extent.fe_flags & FIEMAP_EXTENT_LAST);
        }
        if (last) return;
    }
}

} // namespace

BtrfsInfo readBtrfsInfo(const std::string& path)
{
    BtrfsInfo info;
    struct stat st {};
    if (lstat(path.c_str(), &st) != 0 || !(S_ISREG(st.st_mode) || S_ISDIR(st.st_mode))) return info;

    const int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return info;

    info.known = true;
    btrfs_ioctl_ino_lookup_args lookup {};
    lookup.objectid = kRootDirectoryObject;
    if (ioctl(fd, BTRFS_IOC_INO_LOOKUP, &lookup) == 0) info.subvolumeId = lookup.treeid;

    if (S_ISREG(st.st_mode)) readExtents(fd, info);
    close(fd);

    info.compressionProperty = Xattrs::display(Xattrs::get(path, "btrfs.compression"));
    return info;
}
