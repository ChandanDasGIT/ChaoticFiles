#include "trash/trash.h"

#include "pathbar/pathbar.h"

#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <sys/stat.h>
#include <system_error>

namespace {

std::string trashDirectory()
{
    const char* data = std::getenv("XDG_DATA_HOME");
    const std::string base = data && *data ? data : PathBar::homeDirectory() + "/.local/share";
    return PathBar::normalize(base + "/Trash");
}

bool makePrivate(const std::string& path)
{
    return mkdir(path.c_str(), 0700) == 0 || errno == EEXIST;
}

} // namespace

namespace Trash {

std::string filesDirectory()
{
    return trashDirectory() + "/files";
}

bool ensureExists()
{
    const std::string trash = trashDirectory();
    std::error_code error;
    std::filesystem::create_directories(PathBar::parent(trash), error);
    return !error && makePrivate(trash) && makePrivate(trash + "/files") && makePrivate(trash + "/info");
}

} // namespace Trash
