#include "undo/undostack.h"

namespace {
constexpr std::size_t kMaxSteps = 50;
}

void UndoStack::push(UndoStep step)
{
    m_steps.push_back(std::move(step));
    if (m_steps.size() > kMaxSteps) m_steps.erase(m_steps.begin());
}

UndoStep UndoStack::take()
{
    UndoStep step = std::move(m_steps.back());
    m_steps.pop_back();
    return step;
}
