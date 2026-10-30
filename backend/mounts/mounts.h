#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct Mount {
    std::uint64_t id = 0;
    std::string mountPoint;
    std::string root;                // the part of the filesystem that is mounted here; a btrfs subvolume, say
    std::string options;
    std::string type;
    std::string source;
    std::string superOptions;
};

namespace Mounts {

Mount parseLine(const std::string& line);        // one line of /proc/self/mountinfo
std::vector<Mount> all();
Mount find(std::uint64_t id);                    // an empty Mount if there is none with this id
std::string unescape(const std::string& text);   // mountinfo writes a space as \040

} // namespace Mounts
