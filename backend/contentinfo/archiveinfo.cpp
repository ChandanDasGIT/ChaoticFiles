#include "contentinfo/contentinfo.h"

#include "format/format.h"
#include "process/process.h"

#include <cstdlib>
#include <sstream>

namespace {

std::vector<PropertyRow> summary(std::uint64_t files, std::uint64_t folders, std::uint64_t bytes)
{
    return {{"Files inside", Format::number(files)}, {"Folders inside", Format::number(folders)},
            {"Unpacked size", Format::exactSize(bytes)}};
}

// -rw-r--r-- me/me  1234 2026-10-30 10:00 folder/file.txt
std::vector<PropertyRow> listTar(const std::string& path, const ContentInfo::Cancelled& cancelled)
{
    const auto result = Process::run({"tar", "-tvf", path}, cancelled);
    if (result.exitCode != 0) return {};

    std::uint64_t files = 0, folders = 0, bytes = 0;
    std::istringstream lines(result.output);
    std::string line;
    while (std::getline(lines, line)) {
        std::istringstream fields(line);
        std::string mode, owner;
        std::uint64_t size = 0;
        fields >> mode >> owner >> size;
        if (!mode.empty() && mode[0] == 'd') ++folders;
        else ++files;
        bytes += size;
    }
    return summary(files, folders, bytes);
}

std::vector<PropertyRow> list7z(const std::string& path, const ContentInfo::Cancelled& cancelled)
{
    if (Process::findProgram("7z").empty()) return {{"Archive details", "Not available: 7z is not installed"}};
    const auto result = Process::run({"7z", "l", "-slt", "-p", "--", path}, cancelled);
    if (result.exitCode != 0) return {{"Archive details", "This archive could not be read; it may be damaged or need a password"}};

    // each item is a block of "Key = value" lines ending in an empty line
    std::uint64_t files = 0, folders = 0, bytes = 0;
    bool inList = false, inItem = false, isFolder = false;
    const auto finishItem = [&] {
        if (inItem) ++(isFolder ? folders : files);
        inItem = isFolder = false;
    };
    std::istringstream lines(result.output);
    std::string line;
    while (std::getline(lines, line)) {
        if (line.rfind("----------", 0) == 0) inList = true;
        if (!inList) continue;
        if (line.rfind("Path = ", 0) == 0) { finishItem(); inItem = true; }
        else if (line.rfind("Folder = +", 0) == 0 || line.rfind("Attributes = D", 0) == 0) isFolder = true;
        else if (line.rfind("Size = ", 0) == 0) bytes += std::strtoull(line.c_str() + 7, nullptr, 10);
    }
    finishItem();
    return summary(files, folders, bytes);
}

} // namespace

namespace ContentInfo {

std::vector<PropertyRow> archive(const std::string& path, const std::string& mime, const Cancelled& cancelled)
{
    const bool plainZip = mime == "application/zip" || mime == "application/x-7z-compressed"
        || mime == "application/x-rar" || mime == "application/vnd.rar" || mime == "application/java-archive";
    if (plainZip) return list7z(path, cancelled);

    return listTar(path, cancelled);             // empty for a lone .gz or .xz that holds no tar
}

} // namespace ContentInfo
