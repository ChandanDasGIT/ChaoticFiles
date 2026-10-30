#pragma once
#include <string>
#include <utility>
#include <vector>

// What was done, and how to take it back.
struct UndoStep {
    enum class Kind {
        Renamed,       // pairs: old path, new path
        Moved,         // pairs: where it was, where it is
        Trashed,       // pairs: original path, path in the Trash
        Created,       // pairs: "", the new item (copies and new files; undoing moves them to the Trash)
    };
    Kind kind = Kind::Renamed;
    std::vector<std::pair<std::string, std::string>> pairs;
    std::string description;       // "Rename", "Move 3 items", ...
};

class UndoStack {
public:
    void push(UndoStep step);
    bool canUndo() const { return !m_steps.empty(); }
    const std::string& nextDescription() const { return m_steps.back().description; }
    UndoStep take();

private:
    std::vector<UndoStep> m_steps;
};
