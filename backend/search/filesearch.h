#pragma once
#include "entry/entry.h"

#include <functional>
#include <string>
#include <vector>

namespace FileSearch {

bool nameMatches(const std::string& name, const std::string& query);    // anywhere in the name, ignoring case

// Walks everything under `root` and sends matches in batches. Links are not followed and it stays on one disk.
// Each entry has `location` set to its folder. Stops at `limit` matches.
void run(const std::string& root, const std::string& query, bool showHidden, std::size_t limit,
         const std::function<void(std::vector<Entry>)>& batch, const std::function<bool()>& cancelled);

// The first line of this text with the query in it, ignoring case: "line 12: the words". Empty if there is none,
// or if this is a binary file: one with a zero byte near its start.
std::string findInside(const std::string& text, const std::string& query);

// The same walk, for files with the query in their text. `location` is where it is below `root`, and that line.
void runInside(const std::string& root, const std::string& query, bool showHidden, std::size_t limit,
               const std::function<void(std::vector<Entry>)>& batch, const std::function<bool()>& cancelled);

} // namespace FileSearch
