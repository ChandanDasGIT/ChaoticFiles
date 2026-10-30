#pragma once
#include <string>
#include <vector>

struct Xattr {
    std::string name;
    std::string value;           // raw bytes
};

namespace Xattrs {

std::vector<Xattr> read(const std::string& path);    // every attribute this user may read; does not follow links
std::string get(const std::string& path, const std::string& name);
std::string display(const std::string& value);       // text if it is text, otherwise 0x and hex

} // namespace Xattrs
