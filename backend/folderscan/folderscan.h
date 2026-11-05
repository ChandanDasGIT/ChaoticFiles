#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <utility>
#include <vector>

// Everything inside a folder, added up.
struct FolderScan {
    std::uint64_t files = 0;
    std::uint64_t folders = 0;
    std::uint64_t links = 0;
    std::uint64_t others = 0;
    std::uint64_t hidden = 0;
    std::uint64_t unreadable = 0;              // folders that could not be opened
    std::uint64_t totalSize = 0;
    std::uint64_t diskUsage = 0;               // hard links counted once
    std::vector<std::pair<std::uint64_t, std::string>> largest;   // biggest first, up to 10
    bool finished = false;
};

// Walks the folder without following links. `progress` is called every so often with the totals so far.
// With `oneDisk`, a folder on another disk is left out.
FolderScan scanFolder(const std::string& path, const std::function<void(const FolderScan&)>& progress,
                      const std::function<bool()>& cancelled, bool oneDisk = false);

// The selected items together: files are counted, folders are walked into, and a hard link is counted once.
FolderScan scanItems(const std::vector<std::string>& paths, const std::function<void(const FolderScan&)>& progress,
                     const std::function<bool()>& cancelled);
