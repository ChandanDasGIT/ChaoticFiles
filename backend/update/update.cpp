#include "update/update.h"

extern "C" {
#include "monocypher-ed25519.h"
}

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <sys/file.h>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace {

std::vector<unsigned long> numbers(const std::string& version)
{
    std::vector<unsigned long> parts;
    std::size_t at = 0;
    while (at < version.size() && std::isdigit(static_cast<unsigned char>(version[at]))) {
        unsigned long part = 0;
        for (; at < version.size() && std::isdigit(static_cast<unsigned char>(version[at])); ++at)
            part = part * 10 + static_cast<unsigned long>(version[at] - '0');
        parts.push_back(part);
        if (at < version.size() && version[at] == '.') ++at;
        else break;                                                 // 0.0-dev is 0.0
    }
    while (!parts.empty() && parts.back() == 0) parts.pop_back();   // 1.0 and 1 are the same version
    return parts;
}

bool isHash(const std::string& text)
{
    return text.size() == 64 && std::all_of(text.begin(), text.end(), [](unsigned char c) {
        return std::isdigit(c) || (c >= 'a' && c <= 'f');
    });
}

std::string fromHex(const std::string& hex)
{
    std::string bytes;
    if (hex.size() % 2 != 0) return {};
    const auto digit = [](char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (std::size_t at = 0; at < hex.size(); at += 2) {
        const int high = digit(hex[at]);
        const int low = digit(hex[at + 1]);
        if (high < 0 || low < 0) return {};
        bytes.push_back(static_cast<char>(high * 16 + low));
    }
    return bytes;
}

int instanceLock = -1;

constexpr char kCloseRequestFile[] = "update-closing";
constexpr char kWindowsFolder[] = "update-windows";

// all at once, so that nothing ever reads half a file
void writeWhole(const std::string& path, const std::string& text)
{
    const std::string temporary = path + ".part";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file << text;
    }
    std::rename(temporary.c_str(), path.c_str());
}

int requesterIn(const std::string& folder)
{
    std::ifstream file(folder + "/" + kCloseRequestFile);
    int pid = 0;
    file >> pid;
    return pid;
}

} // namespace

namespace Update {

bool isVersion(const std::string& text)
{
    if (text.empty() || text.front() == '.' || text.back() == '.' || text.find("..") != std::string::npos) return false;
    return std::all_of(text.begin(), text.end(), [](unsigned char c) { return std::isdigit(c) || c == '.'; });
}

bool isNewer(const std::string& version, const std::string& running)
{
    return isVersion(version) && numbers(version) > numbers(running);
}

std::optional<Manifest> parseManifest(const std::string& text)
{
    std::istringstream lines(text);
    std::string header, versionWord, tarballWord, appImageWord, extra;
    Manifest manifest;
    if (!std::getline(lines, header) || header != "ChaoticFiles update manifest 1") return std::nullopt;
    if (!(lines >> versionWord >> manifest.version)) return std::nullopt;
    if (!(lines >> tarballWord >> manifest.tarball >> manifest.tarballHash)) return std::nullopt;
    if (!(lines >> appImageWord >> manifest.appImage >> manifest.appImageHash)) return std::nullopt;
    if (lines >> extra) return std::nullopt;

    if (versionWord != "version" || tarballWord != "tarball" || appImageWord != "appimage") return std::nullopt;
    if (!isVersion(manifest.version)) return std::nullopt;
    // the names can only be these, so a manifest can never point outside the update's folder
    if (manifest.tarball != "ChaoticFiles-" + manifest.version + "-linux-x86_64.tar.xz") return std::nullopt;
    if (manifest.appImage != "ChaoticFiles-" + manifest.version + "-x86_64.AppImage") return std::nullopt;
    if (!isHash(manifest.tarballHash) || !isHash(manifest.appImageHash)) return std::nullopt;
    return manifest;
}

bool signatureIsValid(const std::string& message, const std::string& signature, const std::string& publicKeyHex)
{
    const std::string key = fromHex(publicKeyHex);
    if (key.size() != 32 || signature.size() != 64) return false;
    return crypto_ed25519_check(reinterpret_cast<const uint8_t*>(signature.data()),
                                reinterpret_cast<const uint8_t*>(key.data()),
                                reinterpret_cast<const uint8_t*>(message.data()), message.size()) == 0;
}

const char* installScript()
{
    return R"SCRIPT(#!/bin/sh
stage=$1; new=$2; installed=$3; target=$4; program=$5; oldprogram=$6
shift 6

# the old program leaves this file as the last thing it does
i=0
while [ ! -e "$stage/old-exited" ] && [ $i -lt 100 ]; do sleep 0.1; i=$((i + 1)); done

backup="$stage/previous"
if ! mv "$installed" "$backup"; then
    : > "$stage.failed"
    rm -rf "$stage"
    exec "$oldprogram" "$@"
fi
if ! mv "$new" "$target"; then
    mv "$backup" "$installed"
    : > "$stage.failed"
    rm -rf "$stage"
    exec "$oldprogram" "$@"
fi

started="$stage/started"
exited="$stage/exited"
# its own process group where setsid exists, because an AppImage is a runtime with the app inside it
(
    export CHAOTICFILES_UPDATE_STARTED="$started"
    if command -v setsid > /dev/null 2>&1; then
        setsid --wait sh -c 'echo $$ > "$0"; exec "$@"' "$stage/pid" "$program" "$@"
    else
        sh -c 'echo $$ > "$0"; exec "$@"' "$stage/pid" "$program" "$@"
    fi
    { : > "$exited"; } 2>/dev/null
) &

# one minute, for a slow disk: after that it counts as stuck
deadline=$(($(date +%s) + 60))
while [ ! -e "$started" ] && [ ! -e "$exited" ] && [ "$(date +%s)" -lt "$deadline" ]; do sleep 0.1; done

if [ ! -e "$started" ]; then                                    # it did not start: the old version comes back
    if [ ! -e "$exited" ] && [ -s "$stage/pid" ]; then
        pid=$(cat "$stage/pid")
        kill -s TERM -- "-$pid" 2>/dev/null || kill -s TERM "$pid" 2>/dev/null
        sleep 2
        kill -s KILL -- "-$pid" 2>/dev/null || kill -s KILL "$pid" 2>/dev/null
    fi
    rm -rf "$target"
    mv "$backup" "$installed"
    : > "$stage.failed"
    rm -rf "$stage"
    "$oldprogram" "$@" &
    exit 0
fi
rm -rf "$stage"
)SCRIPT";
}

std::string appImageTarget(const std::string& installedPath, const std::string& oldVersion, const std::string& newName)
{
    const std::size_t slash = installedPath.rfind('/');
    const std::string folder = slash == std::string::npos ? std::string() : installedPath.substr(0, slash + 1);
    const std::string name = slash == std::string::npos ? installedPath : installedPath.substr(slash + 1);
    return name == "ChaoticFiles-" + oldVersion + "-x86_64.AppImage" ? folder + newName : installedPath;
}

bool holdInstanceLock(const std::string& path)
{
    instanceLock = open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, 0600);
    return instanceLock >= 0 && flock(instanceLock, LOCK_SH | LOCK_NB) == 0;
}

bool otherInstancesRunning()
{
    if (instanceLock < 0) return false;
    // a refused change of lock loses the lock already held, so it is always taken back
    const bool others = flock(instanceLock, LOCK_EX | LOCK_NB) != 0;
    flock(instanceLock, LOCK_SH | LOCK_NB);
    return others;
}

void askWindowsToClose(const std::string& folder, int pid)
{
    writeWhole(folder + "/" + kCloseRequestFile, std::to_string(pid));
}

void stopAskingWindowsToClose(const std::string& folder, int pid)
{
    if (requesterIn(folder) == pid) std::remove((folder + "/" + kCloseRequestFile).c_str());
}

int closeRequester(const std::string& folder)
{
    std::error_code error;
    const auto written = std::filesystem::last_write_time(folder + "/" + kCloseRequestFile, error);
    if (error) return 0;
    const auto age = std::filesystem::file_time_type::clock::now() - written;
    return age < std::chrono::seconds(kCloseRequestLifetimeSeconds) ? requesterIn(folder) : 0;
}

void rememberWindow(const std::string& folder, int pid, const std::vector<std::string>& arguments)
{
    std::error_code error;
    std::filesystem::create_directories(folder + "/" + kWindowsFolder, error);
    std::string text;
    for (const std::string& argument : arguments) text.append(argument).push_back('\0');   // a name can hold a new line
    writeWhole(folder + "/" + kWindowsFolder + "/" + std::to_string(pid), text);
}

std::vector<std::vector<std::string>> takeRememberedWindows(const std::string& folder)
{
    std::vector<std::vector<std::string>> windows;
    std::error_code error;
    for (const auto& item : std::filesystem::directory_iterator(folder + "/" + kWindowsFolder, error)) {
        std::ifstream file(item.path(), std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        if (std::remove(item.path().c_str()) != 0) continue;              // another window got there first
        std::vector<std::string> arguments;
        std::size_t start = 0;
        for (std::size_t end = text.find('\0'); end != std::string::npos; end = text.find('\0', start)) {
            arguments.push_back(text.substr(start, end - start));
            start = end + 1;
        }
        if (!arguments.empty()) windows.push_back(std::move(arguments));
    }
    return windows;
}

} // namespace Update
