#include "network/networkshares.h"

#include "process/process.h"

#include <cstdlib>
#include <dirent.h>
#include <map>
#include <sstream>
#include <unistd.h>

namespace {

std::map<std::string, std::string> fieldsOf(const std::string& text)
{
    std::map<std::string, std::string> fields;
    std::istringstream parts(text);
    std::string part;
    while (std::getline(parts, part, ',')) {
        const auto equals = part.find('=');
        if (equals != std::string::npos) fields[part.substr(0, equals)] = part.substr(equals + 1);
    }
    return fields;
}

std::string lastLine(std::string text)
{
    while (!text.empty() && (text.back() == '\n' || text.back() == ' ')) text.pop_back();
    const auto newline = text.rfind('\n');
    return newline == std::string::npos ? text : text.substr(newline + 1);
}

} // namespace

namespace NetworkShares {

std::string mountsFolder()
{
    const char* runtime = std::getenv("XDG_RUNTIME_DIR");
    return (runtime && *runtime ? std::string(runtime) : "/run/user/" + std::to_string(getuid())) + "/gvfs";
}

NetworkShare parseMountName(const std::string& folderName)
{
    NetworkShare share;
    const auto colon = folderName.find(':');
    if (colon == std::string::npos) return share;
    const std::string scheme = folderName.substr(0, colon);
    auto fields = fieldsOf(folderName.substr(colon + 1));

    if (scheme == "smb-share") {
        share.name = fields["share"] + " on " + fields["server"];
        share.uri = "smb://" + fields["server"] + "/" + fields["share"];
    } else if (scheme == "sftp" || scheme == "ftp" || scheme == "dav") {
        share.name = fields["host"];
        share.uri = scheme + "://" + (fields.count("user") ? fields["user"] + "@" : "") + fields["host"]
                  + (fields.count("port") ? ":" + fields["port"] : "") + "/";
    } else {
        return share;
    }
    share.path = mountsFolder() + "/" + folderName;
    return share;
}

std::vector<NetworkShare> list()
{
    std::vector<NetworkShare> shares;
    DIR* dir = opendir(mountsFolder().c_str());
    if (!dir) return shares;
    while (dirent* item = readdir(dir)) {
        NetworkShare share = parseMountName(item->d_name);
        if (!share.uri.empty()) shares.push_back(std::move(share));
    }
    closedir(dir);
    return shares;
}

std::string normalizeAddress(const std::string& address)
{
    std::string text = address;
    while (!text.empty() && text.front() == ' ') text.erase(text.begin());
    while (!text.empty() && text.back() == ' ') text.pop_back();
    if (text.rfind("\\\\", 0) == 0) {                   // the Windows way of writing it
        text = text.substr(2);
        for (char& c : text) if (c == '\\') c = '/';
    }
    return text.find("://") == std::string::npos ? "smb://" + text : text;
}

Outcome connect(const std::string& uri, const std::string& user, const std::string& password,
                const std::function<bool()>& cancelled)
{
    if (Process::findProgram("gio").empty()) return {false, "gio is not installed, so network folders can't be opened."};

    std::vector<std::string> command {"gio", "mount"};
    std::string answers;
    if (user.empty() && password.empty()) {
        command.push_back("-a");                        // anonymous
    } else if (uri.rfind("smb://", 0) == 0) {
        answers = user + "\n\n" + password + "\n";      // user, domain, password
    } else {
        answers = password + "\n";
    }
    command.push_back(uri);

    const auto result = Process::run(command, cancelled, answers);
    if (result.cancelled) return {false, "Cancelled."};
    if (result.exitCode != 0 && result.errors.find("already mounted") == std::string::npos)
        return {false, lastLine(result.errors)};
    return {};
}

Outcome disconnect(const std::string& uri)
{
    const auto result = Process::run({"gio", "mount", "-u", uri});
    if (result.exitCode != 0) return {false, lastLine(result.errors)};
    return {};
}

} // namespace NetworkShares
