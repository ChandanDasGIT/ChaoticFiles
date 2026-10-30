#pragma once
#include <cstdint>
#include <string>

struct DiskSpace {
    std::uint64_t total = 0;
    std::uint64_t available = 0;           // what this user can still write
    bool known = false;
};

DiskSpace readDiskSpace(const std::string& path);
