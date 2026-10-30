#pragma once
#include "entry/entry.h"

#include <string_view>
#include <vector>

enum class SortKey { Name = 0, Modified = 1, Type = 2, Size = 3 };   // the order of the columns

struct SortSettings {
    SortKey key = SortKey::Name;
    bool ascending = true;

    bool operator==(const SortSettings&) const = default;
};

// Natural order: file2 before file10, ignoring case for A-Z. Returns <0, 0 or >0.
int naturalCompare(std::string_view a, std::string_view b);

// Sorts by the settings, folders always first. Ties fall back to the name.
void sortEntries(std::vector<Entry>& entries, const SortSettings& settings);
