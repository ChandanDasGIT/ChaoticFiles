#pragma once
#include "fileops/fileops.h"

#include <cstdint>
#include <string>
#include <vector>

struct DriveDevice {
    std::string device;          // /dev/sdb1
    std::string label;
    std::string type;            // ext4, ntfs, ...
    std::uint64_t size = 0;
    std::string mountPoint;
    std::string kind;            // part, disk, loop, ...

    std::string displayName() const;     // its label, or "32 GiB Volume"
};

// Disks that are plugged in, read with lsblk, and mounted with udisks, which asks for a password when one is needed.
namespace Drives {

std::vector<DriveDevice> parseLsblk(const std::string& text);
std::vector<DriveDevice> unmounted();                          // what could be mounted right now
Outcome mount(const std::string& device, std::string* mountPoint);
Outcome unmount(const std::string& device);

} // namespace Drives
