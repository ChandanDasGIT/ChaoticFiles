#pragma once
#include <string>

// The flags lsattr shows. Only regular files and folders are asked: opening anything else can have side effects.
namespace FileFlags {

struct Flags {
    bool known = false;
    std::string letters;     // "----i---------e-------"
    std::string names;       // "immutable, extents"
};

Flags read(const std::string& path);

} // namespace FileFlags
