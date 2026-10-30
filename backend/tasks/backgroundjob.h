#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>

// Runs work on a background thread that is never waited for, so slow disks can't freeze the caller.
// The work can ask whether it was cancelled, and sends results with post(). A posted function runs on the
// background thread, and never after cancel() or the destructor has returned.
class BackgroundJob {
public:
    class Handle {
    public:
        bool cancelled() const { return m_state->cancelled; }
        void post(const std::function<void()>& deliver) const
        {
            std::lock_guard lock(m_state->gate);
            if (!m_state->cancelled) deliver();
        }

    private:
        friend class BackgroundJob;
        struct State {
            std::atomic<bool> cancelled {false};
            std::mutex gate;
        };
        explicit Handle(std::shared_ptr<State> state) : m_state(std::move(state)) {}
        std::shared_ptr<State> m_state;
    };

    BackgroundJob() = default;
    ~BackgroundJob() { cancel(); }
    BackgroundJob(const BackgroundJob&) = delete;
    BackgroundJob& operator=(const BackgroundJob&) = delete;

    void start(std::function<void(const Handle&)> work);     // cancels the job before it
    void cancel();

private:
    std::shared_ptr<Handle::State> m_state;
};
