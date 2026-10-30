#include "sorting/sorting.h"

#include <algorithm>

namespace {

bool isDigit(char c) { return c >= '0' && c <= '9'; }

char lower(char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 32) : c; }

// compares the digit runs that start at a[i] and b[j], and moves both past them
int compareNumbers(std::string_view a, std::size_t& i, std::string_view b, std::size_t& j)
{
    while (i < a.size() && a[i] == '0') ++i;
    while (j < b.size() && b[j] == '0') ++j;
    const std::size_t startA = i, startB = j;
    while (i < a.size() && isDigit(a[i])) ++i;
    while (j < b.size() && isDigit(b[j])) ++j;

    const std::size_t lengthA = i - startA, lengthB = j - startB;
    if (lengthA != lengthB) return lengthA < lengthB ? -1 : 1;     // more digits is a bigger number
    return a.substr(startA, lengthA).compare(b.substr(startB, lengthB));
}

int compareEntries(const Entry& a, const Entry& b, SortKey key)
{
    switch (key) {
    case SortKey::Size:
        if (a.size != b.size) return a.size < b.size ? -1 : 1;
        break;
    case SortKey::Modified:
        if (a.modified != b.modified) return a.modified < b.modified ? -1 : 1;
        if (a.modifiedNanos != b.modifiedNanos) return a.modifiedNanos < b.modifiedNanos ? -1 : 1;
        break;
    case SortKey::Type:
        if (!a.location.empty() || !b.location.empty()) {
            if (const int c = naturalCompare(a.location, b.location)) return c;
        } else if (const int c = naturalCompare(a.typeName, b.typeName)) {
            return c;
        }
        break;
    case SortKey::Name:
        break;
    }
    return naturalCompare(a.name, b.name);
}

} // namespace

int naturalCompare(std::string_view a, std::string_view b)
{
    std::size_t i = 0, j = 0;
    while (i < a.size() && j < b.size()) {
        if (isDigit(a[i]) && isDigit(b[j])) {
            if (const int c = compareNumbers(a, i, b, j)) return c;
            continue;
        }
        const char x = lower(a[i]), y = lower(b[j]);
        if (x != y) return static_cast<unsigned char>(x) < static_cast<unsigned char>(y) ? -1 : 1;
        ++i;
        ++j;
    }
    if (i < a.size()) return 1;
    if (j < b.size()) return -1;
    return a.compare(b);                  // "File" and "file" still need a fixed order
}

void sortEntries(std::vector<Entry>& entries, const SortSettings& settings)
{
    std::sort(entries.begin(), entries.end(), [&settings](const Entry& a, const Entry& b) {
        if (a.isDirectory() != b.isDirectory()) return a.isDirectory();
        const int c = compareEntries(a, b, settings.key);
        return settings.ascending ? c < 0 : c > 0;
    });
}
