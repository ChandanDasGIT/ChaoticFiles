#include "xattrs/xattrs.h"

#include <cstdio>
#include <sys/xattr.h>

namespace Xattrs {

std::string get(const std::string& path, const std::string& name)
{
    const ssize_t size = lgetxattr(path.c_str(), name.c_str(), nullptr, 0);
    if (size <= 0) return {};
    std::string value(static_cast<std::size_t>(size), '\0');
    const ssize_t got = lgetxattr(path.c_str(), name.c_str(), value.data(), value.size());
    if (got < 0) return {};
    value.resize(static_cast<std::size_t>(got));
    return value;
}

std::vector<Xattr> read(const std::string& path)
{
    std::vector<Xattr> attributes;
    const ssize_t size = llistxattr(path.c_str(), nullptr, 0);
    if (size <= 0) return attributes;
    std::string names(static_cast<std::size_t>(size), '\0');
    const ssize_t got = llistxattr(path.c_str(), names.data(), names.size());
    if (got <= 0) return attributes;

    std::size_t start = 0;
    while (start < static_cast<std::size_t>(got)) {
        const std::string name = names.c_str() + start;
        start += name.size() + 1;
        attributes.push_back({name, get(path, name)});
    }
    return attributes;
}

std::string display(const std::string& value)
{
    std::string text = value;
    if (!text.empty() && text.back() == '\0') text.pop_back();     // SELinux contexts end with a zero byte

    bool printable = true;
    for (const unsigned char c : text)
        if (c < 32 && c != '\t' && c != '\n') printable = false;
    if (printable) return text;

    std::string hex = "0x";
    char byte[3];
    for (const unsigned char c : value) {
        std::snprintf(byte, sizeof byte, "%02x", c);
        hex += byte;
    }
    return hex;
}

} // namespace Xattrs
