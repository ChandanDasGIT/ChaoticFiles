#pragma once
#include <string>
#include <vector>

// The folders one tab has visited: back and forward.
class NavigationHistory {
public:
    explicit NavigationHistory(std::string start) : m_current(std::move(start)) {}

    const std::string& current() const { return m_current; }
    bool canGoBack() const { return !m_back.empty(); }
    bool canGoForward() const { return !m_forward.empty(); }

    void visit(const std::string& path);   // going to the current folder again changes nothing
    bool goBack();
    bool goForward();

private:
    std::string m_current;
    std::vector<std::string> m_back;
    std::vector<std::string> m_forward;
};
