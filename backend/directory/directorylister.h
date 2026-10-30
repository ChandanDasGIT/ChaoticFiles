#pragma once
#include "diskspace/diskspace.h"
#include "entry/entry.h"
#include "sorting/sorting.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct ListResult {
    std::string path;
    std::vector<Entry> entries;          // sorted by `sort`
    SortSettings sort;
    DiskSpace disk;
    bool ok = false;
    std::string error;                   // a sentence for the user when ok is false
};

// Reads and sorts a folder. Blocks. `cancelled` is asked after every entry and may be empty.
ListResult listDirectory(const std::string& path, const SortSettings& sort,
                         const std::function<bool()>& cancelled = {});

// Lists folders on a background thread. Starting a new listing cancels the one before it.
// The thread is never waited for, so a folder on a hanging disk or network cannot freeze the caller.
// The callback runs on the background thread, and never after cancel() or the destructor has returned.
class DirectoryLister {
public:
    using Callback = std::function<void(ListResult)>;

    DirectoryLister() = default;
    ~DirectoryLister() { cancel(); }
    DirectoryLister(const DirectoryLister&) = delete;
    DirectoryLister& operator=(const DirectoryLister&) = delete;

    void start(const std::string& path, const SortSettings& sort, Callback callback);
    void cancel();

private:
    struct Job;
    std::shared_ptr<Job> m_job;
};
