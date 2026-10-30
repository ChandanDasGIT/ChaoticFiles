#pragma once
#include <string>
#include <vector>

struct PathPart {
    std::string label;
    std::string path;
};

// Everything the address bar needs that is not drawing. Paths here are absolute and have no trailing "/",
// or they are a location that is not one folder on disk: the Trash, or Recent.
namespace PathBar {

inline constexpr const char* kTrashLocation = "trash:///";
inline constexpr const char* kRecentLocation = "recent:///";
bool isVirtual(const std::string& path);

std::string homeDirectory();
std::string normalize(const std::string& path);    // removes "." and ".." and repeated or trailing "/"
std::string parent(const std::string& path);       // the parent folder; "/" stays "/"
std::string baseName(const std::string& path);
std::string tabTitle(const std::string& path);     // "Home", "Trash" and "/" for those folders

bool isDirectory(const std::string& path);

// "/home/me" gives "/", "home" and "me", each with the path it goes to.
std::vector<PathPart> split(const std::string& path);

// The folders directly inside `path`, in natural order.
std::vector<PathPart> subfolders(const std::string& path, bool showHidden);

// What the user typed, as an absolute path: handles ~, relative paths and file:// addresses.
std::string resolve(const std::string& typed, const std::string& currentFolder);

// Folders that complete what the user has typed so far.
std::vector<PathPart> suggestions(const std::string& typed, const std::string& currentFolder,
                                  bool showHidden, std::size_t limit);

} // namespace PathBar
