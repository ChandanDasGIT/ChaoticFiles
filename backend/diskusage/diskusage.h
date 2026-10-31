#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// What takes up the space in one folder, biggest first.
struct UsageItem {
    std::string name;
    std::string path;
    std::uint64_t bytes = 0;                   // on the disk, with everything inside it
    std::uint64_t files = 0;
    bool folder = false;
};

struct UsageReport {
    std::vector<UsageItem> items;              // the biggest ones, up to kMaxUsageItems
    std::uint64_t total = 0;                   // everything so far, listed or not
    std::uint64_t hiddenItems = 0;             // how many didn't fit in `items`
    std::uint64_t unreadable = 0;              // folders that could not be opened
    std::size_t done = 0;
    std::size_t count = 0;
    std::string current;                       // the one being added up
    bool finished = false;
};

inline constexpr std::size_t kMaxUsageItems = 500;

// Adds up each item in the folder. Stays on one disk and follows no links, so / doesn't count /proc or other drives.
// Hard links are counted once inside each subfolder. `progress` gets the report so far, every so often.
UsageReport scanUsage(const std::string& path, const std::function<void(const UsageReport&)>& progress,
                      const std::function<bool()>& cancelled);
