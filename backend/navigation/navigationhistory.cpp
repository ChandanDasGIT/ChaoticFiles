#include "navigation/navigationhistory.h"

void NavigationHistory::visit(const std::string& path)
{
    if (path == m_current) return;
    m_back.push_back(std::move(m_current));
    m_current = path;
    m_forward.clear();
}

bool NavigationHistory::goBack()
{
    if (m_back.empty()) return false;
    m_forward.push_back(std::move(m_current));
    m_current = std::move(m_back.back());
    m_back.pop_back();
    return true;
}

bool NavigationHistory::goForward()
{
    if (m_forward.empty()) return false;
    m_back.push_back(std::move(m_current));
    m_current = std::move(m_forward.back());
    m_forward.pop_back();
    return true;
}
