#pragma once
#include "entry/entry.h"
#include "sorting/sorting.h"

#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

enum class ClickMode { Replace, Toggle, Extend };     // plain click, Ctrl+click, Shift+click

// What one tab shows: the entries of its folder after sorting and hiding, and the selection.
// The selection is kept by path, so it survives a refresh or a new sort order.
class FolderView {
public:
    void setEntries(std::vector<Entry> entries, const SortSettings& sortedBy);
    void clear();
    bool matches(const std::vector<Entry>& entries, const SortSettings& sortedBy) const;

    const SortSettings& sort() const { return m_sort; }
    void setSort(const SortSettings& sort);
    void setShowHidden(bool show);
    void setFilter(const std::string& text);             // only names containing this are shown
    void addEntries(std::vector<Entry> entries);         // more results of a search; sorts everything again

    int count() const { return static_cast<int>(m_rows.size()); }
    const Entry& at(int row) const { return m_all[static_cast<std::size_t>(m_rows[static_cast<std::size_t>(row)])]; }

    int currentRow() const { return m_currentRow; }
    bool isSelected(int row) const;
    int selectedCount() const { return static_cast<int>(m_selected.size()); }
    std::uint64_t selectedSize() const;                  // files only; folders count as nothing here
    std::vector<std::string> selectedPaths() const;      // in row order

    void click(int row, ClickMode mode);
    void moveCurrent(int row, bool extend);              // the keyboard: `row` is clamped to the list
    void selectAll();
    void selectRows(const std::vector<int>& rows, bool add);     // rubber band: these rows, added or instead
    void clearSelection();
    bool selectName(const std::string& name);
    void setCutPaths(std::unordered_set<std::string> paths) { m_cut = std::move(paths); }   // shown faded
    bool isCut(int row) const { return !m_cut.empty() && m_cut.count(at(row).path) > 0; }

    // the first row at or after `from` whose name starts with `prefix`, wrapping around; -1 if none
    int findPrefix(const std::string& prefix, int from) const;

private:
    void rebuildRows();
    int rowOfPath(const std::string& path) const;
    void setCurrent(int row);

    std::vector<Entry> m_all;
    std::vector<int> m_rows;                             // indexes into m_all of the rows that are shown
    SortSettings m_sort;
    bool m_showHidden = false;
    std::string m_filter;

    std::unordered_set<std::string> m_selected;
    std::unordered_set<std::string> m_cut;
    std::string m_currentPath;
    std::string m_anchorPath;                            // where a Shift range starts
    int m_currentRow = -1;
};
