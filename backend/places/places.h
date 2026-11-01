#pragma once
#include <string>
#include <vector>

struct Place {
    std::string name;
    std::string path;
    std::string kind;       // "home", "recent", "desktop", "documents", "downloads", "music", "pictures", "videos", "root", "trash", "drive"
    std::string device = {};  // a drive's /dev/sdb1
};

namespace Places {

// Home, the standard user folders that exist, the root of the file system, and the Trash.
std::vector<Place> userPlaces();

// Mounted drives that are worth showing: real devices, without the system's own mounts.
std::vector<Place> drives();

// One line of /proc/self/mountinfo as a drive, or an empty path if it should not be shown.
Place driveFromMountInfo(const std::string& line);

// The pinned folders after `moved` are put before position `index` of `pins`. A folder that is already pinned
// moves instead of showing twice. An index past the end means the end.
std::vector<std::string> placePins(const std::vector<std::string>& pins, const std::vector<std::string>& moved,
                                   std::size_t index);

} // namespace Places
