#include "tasks/backgroundjob.h"

#include <thread>

void BackgroundJob::start(std::function<void(const Handle&)> work)
{
    cancel();
    auto state = std::make_shared<Handle::State>();
    m_state = state;
    std::thread([state, work = std::move(work)] { work(Handle(state)); }).detach();
}

void BackgroundJob::cancel()
{
    if (!m_state) return;
    m_state->cancelled = true;
    std::lock_guard lock(m_state->gate);   // a delivery that already started finishes first
    m_state.reset();
}
