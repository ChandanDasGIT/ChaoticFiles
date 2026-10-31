#include "folderview/folderview.h"

#include "search/filesearch.h"

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
    m_currentPath.clear();
    m_anchorPath.clear();
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

void FolderView::setFilter(const std::string& text)
{
    m_filter = text;
    rebuildRows();
}

void FolderView::addEntries(std::vector<Entry> entries)
{
    m_all.insert(m_all.end(), std::make_move_iterator(entries.begin()), std::make_move_iterator(entries.end()));
    sortEntries(m_all, m_sort);
    rebuildRows();
}

void FolderView::rebuildRows()
{
    m_rows.clear();
    m_rows.reserve(m_all.size());
    for (std::size_t i = 0; i < m_all.size(); ++i) {
        if (!m_showHidden && m_all[i].hidden) continue;
        if (!m_filter.empty() && !FileSearch::nameMatches(m_all[i].name, m_filter)) continue;
        m_rows.push_back(static_cast<int>(i));
    }

    // anything that left the list leaves the selection too
    std::unordered_set<std::string> kept;
    m_currentRow = -1;
    for (int row = 0; row < count(); ++row) {
        const std::string& path = at(row).path;
        if (m_selected.count(path)) kept.insert(path);
        if (path == m_currentPath) m_currentRow = row;
    }
    m_selected = std::move(kept);
    if (m_currentRow < 0) m_currentPath.clear();
}

int FolderView::rowOfPath(const std::string& path) const
{
    for (int row = 0; row < count(); ++row)
        if (at(row).path == path) return row;
    return -1;
}

void FolderView::setCurrent(int row)
{
    m_currentRow = row;
    m_currentPath = row >= 0 ? at(row).path : std::string();
}

bool FolderView::isSelected(int row) const
{
    return !m_selected.empty() && m_selected.count(at(row).path) > 0;
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
    const std::string& path = at(row).path;

    switch (mode) {
    case ClickMode::Replace:
        m_selected.clear();
        m_selected.insert(path);
        m_anchorPath = path;
        break;
    case ClickMode::Toggle:
        if (!m_selected.erase(path)) m_selected.insert(path);
        m_anchorPath = path;
        break;
    case ClickMode::Extend: {
        int anchor = rowOfPath(m_anchorPath);
        if (anchor < 0) anchor = m_currentRow >= 0 ? m_currentRow : row;
        m_anchorPath = at(anchor).path;
        m_selected.clear();
        for (int r = std::min(anchor, row); r <= std::max(anchor, row); ++r)
            m_selected.insert(at(r).path);
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
    for (int row = 0; row < count(); ++row) m_selected.insert(at(row).path);
}

void FolderView::selectRows(const std::vector<int>& rows, bool add)
{
    if (!add) m_selected.clear();
    for (const int row : rows)
        if (row >= 0 && row < count()) m_selected.insert(at(row).path);
}

void FolderView::clearSelection()
{
    m_selected.clear();
    m_anchorPath.clear();
}

bool FolderView::selectName(const std::string& name)
{
    for (int row = 0; row < count(); ++row) {
        if (at(row).name == name) {
            click(row, ClickMode::Replace);
            return true;
        }
    }
    return false;
}

int FolderView::findTyped(const std::string& typed, int current) const
{
    if (typed.empty() || count() == 0) return -1;

    // the first letter, which can be several bytes in UTF-8
    std::size_t letter = 1;
    while (letter < typed.size() && (static_cast<unsigned char>(typed[letter]) & 0xC0) == 0x80) ++letter;

    bool oneLetter = true;
    for (std::size_t at = 0; at < typed.size() && oneLetter; ++at)
        oneLetter = std::tolower(static_cast<unsigned char>(typed[at])) == std::tolower(static_cast<unsigned char>(typed[at % letter]));

    if (oneLetter) return findPrefix(typed.substr(0, letter), current + 1 >= count() ? 0 : current + 1);
    return findPrefix(typed, std::max(current, 0));
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
