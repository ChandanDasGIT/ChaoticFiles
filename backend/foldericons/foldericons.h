#pragma once
#include "fileops/fileops.h"

#include <string>

// The icon someone chose for a folder. It is kept in an attribute on the folder itself, so it follows the folder
// through renames and moves, and leaves no file behind. It holds "icon;colour", the names the interface uses.
namespace FolderIcons {

std::string read(const std::string& path);           // empty if the folder has none or can't hold one; follows links
Outcome write(const std::string& path, const std::string& value);     // empty value removes it; anything but a folder is skipped

std::string iconOf(const std::string& value);
std::string colorOf(const std::string& value);
std::string make(const std::string& icon, const std::string& color);

} // namespace FolderIcons
