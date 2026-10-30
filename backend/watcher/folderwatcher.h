#pragma once
#include <functional>
#include <string>
#include <thread>

// Calls back when anything inside one folder changes. Bursts are grouped: the callback comes once
// things go quiet for a moment, or after a second at the latest. It runs on the watcher's own thread,
// and never after stop() or the destructor has returned.
class FolderWatcher {
public:
    FolderWatcher() = default;
    ~FolderWatcher() { stop(); }
    FolderWatcher(const FolderWatcher&) = delete;
    FolderWatcher& operator=(const FolderWatcher&) = delete;

    void watch(const std::string& path, std::function<void()> onChange);
    void stop();

private:
    std::thread m_thread;
    int m_wakeFd = -1;
};
