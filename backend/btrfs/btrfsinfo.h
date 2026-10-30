#pragma once
#include <cstdint>
#include <string>

// What can be learned about a file on btrfs without being root.
struct BtrfsInfo {
    bool known = false;
    std::uint64_t subvolumeId = 0;
    std::string compressionProperty;    // the btrfs.compression attribute, if set
    std::uint64_t extents = 0;
    std::uint64_t compressedExtents = 0;
    std::uint64_t logicalBytes = 0;
    std::uint64_t compressedLogicalBytes = 0;
    std::uint64_t sharedBytes = 0;      // also used by a snapshot or a reflink copy
    std::uint64_t exclusiveBytes = 0;
};

BtrfsInfo readBtrfsInfo(const std::string& path);
