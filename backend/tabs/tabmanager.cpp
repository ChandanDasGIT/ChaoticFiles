#include "tabs/tabmanager.h"

#include <algorithm>

namespace {
constexpr std::size_t kMaxClosedTabs = 20;
}

std::shared_ptr<Tab> TabManager::open(const std::string& path, int index)
{
    auto tab = std::make_shared<Tab>(path);
    const int place = std::clamp(index, 0, count());
    m_tabs.insert(m_tabs.begin() + place, tab);
    if (m_current < 0) m_current = 0;
    else if (place <= m_current) ++m_current;
    return tab;
}

void TabManager::close(int index)
{
    if (index < 0 || index >= count()) return;
    m_closed.push_back({m_tabs[static_cast<std::size_t>(index)], index});
    if (m_closed.size() > kMaxClosedTabs) m_closed.erase(m_closed.begin());

    m_tabs.erase(m_tabs.begin() + index);
    if (m_tabs.empty()) m_current = -1;
    else if (index < m_current) --m_current;
    else if (m_current >= count()) m_current = count() - 1;
}

void TabManager::setCurrent(int index)
{
    if (index >= 0 && index < count()) m_current = index;
}

void TabManager::move(int from, int to)
{
    if (from < 0 || to < 0 || from >= count() || to >= count() || from == to) return;
    auto tab = std::move(m_tabs[static_cast<std::size_t>(from)]);
    m_tabs.erase(m_tabs.begin() + from);
    m_tabs.insert(m_tabs.begin() + to, std::move(tab));

    if (m_current == from) m_current = to;
    else if (from < m_current && to >= m_current) --m_current;
    else if (from > m_current && to <= m_current) ++m_current;
}

int TabManager::reopenIndex() const
{
    return m_closed.empty() ? -1 : std::min(m_closed.back().index, count());
}

std::shared_ptr<Tab> TabManager::reopen()
{
    if (m_closed.empty()) return nullptr;
    const int place = reopenIndex();
    auto tab = std::move(m_closed.back().tab);
    m_closed.pop_back();
    m_tabs.insert(m_tabs.begin() + place, tab);
    if (m_current < 0) m_current = 0;
    else if (place <= m_current) ++m_current;
    return tab;
}
