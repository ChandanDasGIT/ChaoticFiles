#pragma once
#include <string>
#include <vector>

// Files opened lately: this app's own list, plus the list other Linux apps keep in recently-used.xbel,
// which is only read.
namespace RecentFiles {

void add(const std::string& path);
std::vector<std::string> list();                       // newest first, only files that still exist
void clear();                                          // this app's own list
std::vector<std::string> parseXbel(const std::string& text);    // the file paths in an xbel document

} // namespace RecentFiles
