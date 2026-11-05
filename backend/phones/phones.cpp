#include "phones/phones.h"

#include "network/networkshares.h"
#include "process/process.h"

#include <sstream>
#include <sys/stat.h>

namespace {

std::string trimmed(const std::string& text)
{
    const auto first = text.find_first_not_of(" \t\r");
    if (first == std::string::npos) return {};
    return text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}

bool isFolder(const std::string& path)
{
    struct stat info {};
    return stat(path.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
}

std::string lastLine(std::string text)
{
    while (!text.empty() && (text.back() == '\n' || text.back() == ' ')) text.pop_back();
    const auto newline = text.rfind('\n');
    return newline == std::string::npos ? text : text.substr(newline + 1);
}

} // namespace

namespace Phones {

bool isPhoneUri(const std::string& uri)
{
    return uri.rfind("mtp://", 0) == 0 || uri.rfind("gphoto2://", 0) == 0;
}

std::string folderName(const std::string& uri)
{
    const auto colon = uri.find("://");
    if (colon == std::string::npos) return {};
    std::string host = uri.substr(colon + 3);
    while (!host.empty() && host.back() == '/') host.pop_back();
    return uri.substr(0, colon) + ":host=" + host;
}

// Volume blocks look like:
//   Volume(0): POCO X3 Pro
//     Type: GProxyVolume (GProxyVolumeMonitorMTP)
//     activation_root=mtp://Xiaomi_POCO_X3_Pro_6a91fef5/
std::vector<Phone> parseGioList(const std::string& text, const std::string& gvfsFolder)
{
    std::vector<Phone> phones;
    std::string name;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        const std::string item = trimmed(line);
        if (item.rfind("Volume(", 0) == 0) {
            const auto colon = item.find("): ");
            name = colon == std::string::npos ? std::string() : item.substr(colon + 3);
        } else if (item.rfind("Drive(", 0) == 0 || item.rfind("Mount(", 0) == 0) {
            if (line.find_first_not_of(' ') == 0) name.clear();     // a new block at the top level
        } else if (item.rfind("activation_root=", 0) == 0 && !name.empty()) {
            const std::string uri = item.substr(16);
            if (!isPhoneUri(uri)) continue;
            bool seen = false;
            for (const Phone& phone : phones) seen = seen || phone.uri == uri;
            if (!seen) phones.push_back({name, uri, gvfsFolder + "/" + folderName(uri)});
            name.clear();
        }
    }
    return phones;
}

std::vector<Phone> list()
{
    if (Process::findProgram("gio").empty()) return {};
    const auto result = Process::run({"gio", "mount", "-li"});
    std::vector<Phone> phones = parseGioList(result.output, NetworkShares::mountsFolder());
    for (Phone& phone : phones)
        if (!isFolder(phone.path)) phone.path.clear();             // not mounted yet
    return phones;
}

Outcome mount(const std::string& uri, std::string* path)
{
    if (Process::findProgram("gio").empty()) return {false, "gio is not installed, so phones can't be opened."};
    const auto result = Process::run({"gio", "mount", uri});
    if (result.exitCode != 0 && result.errors.find("already mounted") == std::string::npos)
        return {false, lastLine(result.errors) + "\n\nUnlock the phone and choose \"File transfer\" in its USB menu, then try again."};
    const std::string folder = NetworkShares::mountsFolder() + "/" + folderName(uri);
    if (!isFolder(folder)) return {false, "The phone was opened, but its folder did not appear."};
    if (path) *path = folder;
    return {};
}

Outcome unmount(const std::string& uri)
{
    const auto result = Process::run({"gio", "mount", "-u", uri});
    if (result.exitCode != 0) return {false, lastLine(result.errors)};
    return {};
}

} // namespace Phones
