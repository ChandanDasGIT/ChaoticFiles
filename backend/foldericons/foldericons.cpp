#include "foldericons/foldericons.h"

#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/xattr.h>

namespace {

constexpr const char* kAttribute = "user.chaoticfiles.icon";
constexpr std::size_t kMaxLength = 64;

std::string errorSentence(int error)
{
    switch (error) {
    case ENOTSUP: return "This drive can't store folder icons.";
    case EACCES:
    case EPERM:
    case EROFS: return "You don't have permission to change this folder.";
    default: return std::string("The folder icon was not saved: ") + std::strerror(error) + ".";
    }
}

} // namespace

namespace FolderIcons {

std::string read(const std::string& path)
{
    char value[kMaxLength];
    const ssize_t got = getxattr(path.c_str(), kAttribute, value, sizeof value);
    return got > 0 ? std::string(value, static_cast<std::size_t>(got)) : std::string();
}

Outcome write(const std::string& path, const std::string& value)
{
    struct stat info {};
    if (stat(path.c_str(), &info) != 0) return {false, errorSentence(errno)};
    if (!S_ISDIR(info.st_mode)) return {};

    if (value.empty()) {
        if (removexattr(path.c_str(), kAttribute) != 0 && errno != ENODATA) return {false, errorSentence(errno)};
    } else if (setxattr(path.c_str(), kAttribute, value.data(), value.size(), 0) != 0) {
        return {false, errorSentence(errno)};
    }
    return {};
}

std::string iconOf(const std::string& value)
{
    return value.substr(0, value.find(';'));
}

std::string colorOf(const std::string& value)
{
    const auto semicolon = value.find(';');
    return semicolon == std::string::npos ? std::string() : value.substr(semicolon + 1);
}

std::string make(const std::string& icon, const std::string& color)
{
    return icon.empty() && color.empty() ? std::string() : icon + ";" + color;
}

} // namespace FolderIcons
