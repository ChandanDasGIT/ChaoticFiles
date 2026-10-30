#pragma once
#include "folderview/folderview.h"
#include "navigation/navigationhistory.h"

#include <memory>
#include <string>
#include <vector>

// One tab's own state: where it is, where it has been, how it sorts and what is selected.
struct Tab {
    explicit Tab(const std::string& path) : history(path) {}

    NavigationHistory history;
    FolderView view;
};

// Every open tab, in order, and the ones that can be reopened.
class TabManager {
public:
    int count() const { return static_cast<int>(m_tabs.size()); }
    int current() const { return m_current; }
    const std::shared_ptr<Tab>& at(int index) const { return m_tabs[static_cast<std::size_t>(index)]; }

    std::shared_ptr<Tab> open(const std::string& path, int index);   // does not change the current tab
    void close(int index);                                           // the caller keeps at least one tab
    void setCurrent(int index);
    void move(int from, int to);

    bool canReopen() const { return !m_closed.empty(); }
    int reopenIndex() const;
    std::shared_ptr<Tab> reopen();                                   // back in its old place, history included

private:
    struct ClosedTab {
        std::shared_ptr<Tab> tab;
        int index;
    };

    std::vector<std::shared_ptr<Tab>> m_tabs;
    std::vector<ClosedTab> m_closed;
    int m_current = -1;
};
