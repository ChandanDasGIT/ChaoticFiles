#include "watcher/folderwatcher.h"

#include <algorithm>
#include <chrono>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/inotify.h>
#include <unistd.h>

namespace {

constexpr int kQuietMs = 250;
constexpr int kLongestWaitMs = 1000;

void watchLoop(int inotifyFd, int wakeFd, const std::function<void()>& onChange)
{
    using Clock = std::chrono::steady_clock;
    pollfd fds[2] = {{inotifyFd, POLLIN, 0}, {wakeFd, POLLIN, 0}};
    bool pending = false;
    Clock::time_point firstChange;
    char buffer[8192];

    while (true) {
        int timeout = -1;
        if (pending) {
            const auto waited = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - firstChange).count();
            timeout = std::clamp(kLongestWaitMs - static_cast<int>(waited), 0, kQuietMs);
        }
        const int ready = poll(fds, 2, timeout);
        if (ready < 0) continue;
        if (fds[1].revents) return;

        if (ready > 0 && (fds[0].revents & POLLIN)) {
            if (read(inotifyFd, buffer, sizeof buffer) <= 0) return;
            if (!pending) {
                pending = true;
                firstChange = Clock::now();
            }
            if (Clock::now() - firstChange < std::chrono::milliseconds(kLongestWaitMs)) continue;
        }
        if (pending) {
            pending = false;
            onChange();
        }
    }
}

} // namespace

void FolderWatcher::watch(const std::string& path, std::function<void()> onChange)
{
    stop();
    const int inotifyFd = inotify_init1(IN_CLOEXEC);
    if (inotifyFd < 0) return;

    const unsigned events = IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB | IN_CLOSE_WRITE
                          | IN_DELETE_SELF | IN_MOVE_SELF | IN_ONLYDIR;
    if (inotify_add_watch(inotifyFd, path.c_str(), events) < 0) {
        close(inotifyFd);
        return;
    }
    m_wakeFd = eventfd(0, EFD_CLOEXEC);
    if (m_wakeFd < 0) {
        close(inotifyFd);
        return;
    }
    m_thread = std::thread([inotifyFd, wakeFd = m_wakeFd, onChange = std::move(onChange)] {
        watchLoop(inotifyFd, wakeFd, onChange);
        close(inotifyFd);
    });
}

void FolderWatcher::stop()
{
    if (m_wakeFd < 0) return;
    const std::uint64_t one = 1;
    [[maybe_unused]] const auto written = write(m_wakeFd, &one, sizeof one);
    m_thread.join();
    close(m_wakeFd);
    m_wakeFd = -1;
}
