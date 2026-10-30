#pragma once
#include "fileops/fileops.h"

#include <cstdint>
#include <string>
#include <vector>

struct TrashItem {
    std::string name;              // the name it had
    std::string path;              // where it is now, inside a Trash
    std::string originalPath;
    std::int64_t deletedAt = 0;    // seconds since 1970
};

// The Trash that Linux desktops share (the freedesktop.org Trash specification). Home has one, and every other
// disk has its own at its top, because moving a file there would otherwise mean copying it.
namespace Trash {

std::string filesDirectory();      // where trashed files are, usually ~/.local/share/Trash/files
bool ensureExists();               // creates the Trash if this user has none yet

Outcome moveToTrash(const std::string& path, std::string* trashedPath = nullptr);
Outcome restore(const std::string& trashedPath, std::string* restoredTo = nullptr);
Outcome remove(const std::string& trashedPath);       // deletes one trashed item for good
std::vector<TrashItem> list();                        // every Trash on every disk
Outcome empty();
bool contains(const std::string& path);               // whether this path is inside any Trash

} // namespace Trash
