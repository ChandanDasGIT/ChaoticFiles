#pragma once
#include <string>

// The Trash that Linux desktops share (the freedesktop.org Trash specification).
namespace Trash {

std::string filesDirectory();      // where trashed files are, usually ~/.local/share/Trash/files
bool ensureExists();               // creates the Trash if this user has none yet

} // namespace Trash
