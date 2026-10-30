#include "folderview/folderview.h"

#include <algorithm>
#include <cctype>

void FolderView::setEntries(std::vector<Entry> entries, const SortSettings& sortedBy)
{
    m_all = std::move(entries);
    if (!(sortedBy == m_sort)) sortEntries(m_all, m_sort);
    rebuildRows();
}

void FolderView::clear()
{
    m_all.clear();
    m_selected.clear();
    m_currentName.clear();
    m_anchorName.clear();
    rebuildRows();
}

bool FolderView::matches(const std::vector<Entry>& entries, const SortSettings& sortedBy) const
{
    return sortedBy == m_sort && entries.size() == m_all.size()
        && std::equal(entries.begin(), entries.end(), m_all.begin(), sameOnDisk);
}

void FolderView::setSort(const SortSettings& sort)
{
    m_sort = sort;
    sortEntries(m_all, m_sort);
    rebuildRows();
}

void FolderView::setShowHidden(bool show)
{
    m_showHidden = show;
    rebuildRows();
}

void FolderView::rebuildRows()
{
    m_rows.clear();
    m_rows.reserve(m_all.size());
    for (std::size_t i = 0; i < m_all.size(); ++i)
        if (m_showHidden || !m_all[i].hidden) m_rows.push_back(static_cast<int>(i));

    // anything that left the list leaves the selection too
    std::unordered_set<std::string> kept;
    m_currentRow = -1;
    for (int row = 0; row < count(); ++row) {
        const std::string& name = at(row).name;
        if (m_selected.count(name)) kept.insert(name);
        if (name == m_currentName) m_currentRow = row;
    }
    m_selected = std::move(kept);
    if (m_currentRow < 0) m_currentName.clear();
}

int FolderView::rowOfName(const std::string& name) const
{
    for (int row = 0; row < count(); ++row)
        if (at(row).name == name) return row;
    return -1;
}

void FolderView::setCurrent(int row)
{
    m_currentRow = row;
    m_currentName = row >= 0 ? at(row).name : std::string();
}

bool FolderView::isSelected(int row) const
{
    return !m_selected.empty() && m_selected.count(at(row).name) > 0;
}

std::uint64_t FolderView::selectedSize() const
{
    std::uint64_t total = 0;
    if (m_selected.empty()) return total;
    for (int row = 0; row < count(); ++row)
        if (!at(row).isDirectory() && isSelected(row)) total += at(row).size;
    return total;
}

std::vector<std::string> FolderView::selectedPaths() const
{
    std::vector<std::string> paths;
    for (int row = 0; row < count(); ++row)
        if (isSelected(row)) paths.push_back(at(row).path);
    return paths;
}

void FolderView::click(int row, ClickMode mode)
{
    if (row < 0 || row >= count()) return;
    const std::string& name = at(row).name;

    switch (mode) {
    case ClickMode::Replace:
        m_selected.clear();
        m_selected.insert(name);
        m_anchorName = name;
        break;
    case ClickMode::Toggle:
        if (!m_selected.erase(name)) m_selected.insert(name);
        m_anchorName = name;
        break;
    case ClickMode::Extend: {
        int anchor = rowOfName(m_anchorName);
        if (anchor < 0) anchor = m_currentRow >= 0 ? m_currentRow : row;
        m_anchorName = at(anchor).name;
        m_selected.clear();
        for (int r = std::min(anchor, row); r <= std::max(anchor, row); ++r)
            m_selected.insert(at(r).name);
        break;
    }
    }
    setCurrent(row);
}

void FolderView::moveCurrent(int row, bool extend)
{
    if (count() == 0) return;
    click(std::clamp(row, 0, count() - 1), extend ? ClickMode::Extend : ClickMode::Replace);
}

void FolderView::selectAll()
{
    for (int row = 0; row < count(); ++row) m_selected.insert(at(row).name);
}

void FolderView::clearSelection()
{
    m_selected.clear();
    m_anchorName.clear();
}

bool FolderView::selectName(const std::string& name)
{
    const int row = rowOfName(name);
    if (row < 0) return false;
    click(row, ClickMode::Replace);
    return true;
}

int FolderView::findPrefix(const std::string& prefix, int from) const
{
    if (prefix.empty() || count() == 0) return -1;
    const auto startsWith = [&prefix](const std::string& name) {
        return name.size() >= prefix.size()
            && std::equal(prefix.begin(), prefix.end(), name.begin(), [](unsigned char a, unsigned char b) {
                   return std::tolower(a) == std::tolower(b);
               });
    };
    const int start = std::clamp(from, 0, count() - 1);
    for (int i = 0; i < count(); ++i) {
        const int row = (start + i) % count();
        if (startsWith(at(row).name)) return row;
    }
    return -1;
}
