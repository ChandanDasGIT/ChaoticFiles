#include "drives/drives.h"

#include "format/format.h"
#include "process/process.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

namespace {

// lines look like: PATH="/dev/sda1" LABEL="My Disk" FSTYPE="ext4" SIZE="1024" MOUNTPOINT="" TYPE="part"
std::string fieldOf(const std::string& line, const std::string& name)
{
    const std::string start = name + "=\"";
    auto at = line.find(start);
    while (at != std::string::npos && at != 0 && line[at - 1] != ' ') at = line.find(start, at + 1);
    if (at == std::string::npos) return {};
    at += start.size();
    const auto end = line.find('"', at);
    return end == std::string::npos ? std::string() : line.substr(at, end - at);
}

bool canMount(const DriveDevice& drive)
{
    static const char* const skipped[] = {"swap", "LVM2_member", "crypto_LUKS", "zfs_member", "linux_raid_member"};
    if (drive.type.empty() || !drive.mountPoint.empty()) return false;
    if (drive.kind != "part" && drive.kind != "disk") return false;
    return std::none_of(std::begin(skipped), std::end(skipped), [&drive](const char* type) { return drive.type == type; });
}

std::string lastLine(std::string text)
{
    while (!text.empty() && (text.back() == '\n' || text.back() == ' ')) text.pop_back();
    const auto newline = text.rfind('\n');
    return newline == std::string::npos ? text : text.substr(newline + 1);
}

} // namespace

std::string DriveDevice::displayName() const
{
    return label.empty() ? Format::size(size) + " Volume" : label;
}

namespace Drives {

std::vector<DriveDevice> parseLsblk(const std::string& text)
{
    std::vector<DriveDevice> drives;
    std::istringstream lines(text);
    std::string line;
    while (std::getline(lines, line)) {
        DriveDevice drive;
        drive.device = fieldOf(line, "PATH");
        drive.label = fieldOf(line, "LABEL");
        drive.type = fieldOf(line, "FSTYPE");
        drive.size = std::strtoull(fieldOf(line, "SIZE").c_str(), nullptr, 10);
        drive.mountPoint = fieldOf(line, "MOUNTPOINT");
        drive.kind = fieldOf(line, "TYPE");
        if (!drive.device.empty()) drives.push_back(std::move(drive));
    }
    return drives;
}

std::vector<DriveDevice> unmounted()
{
    const auto result = Process::run({"lsblk", "-P", "-b", "-o", "PATH,LABEL,FSTYPE,SIZE,MOUNTPOINT,TYPE"});
    std::vector<DriveDevice> drives;
    for (DriveDevice& drive : parseLsblk(result.output))
        if (canMount(drive)) drives.push_back(std::move(drive));
    return drives;
}

Outcome mount(const std::string& device, std::string* mountPoint)
{
    const auto result = Process::run({"udisksctl", "mount", "-b", device});
    if (!result.started) return {false, "udisksctl is not installed."};
    if (result.exitCode != 0) return {false, lastLine(result.errors)};

    // "Mounted /dev/sdb1 at /run/media/me/USB"
    const std::string marker = " at ";
    const auto at = result.output.find(marker);
    if (mountPoint && at != std::string::npos) {
        *mountPoint = result.output.substr(at + marker.size());
        while (!mountPoint->empty() && (mountPoint->back() == '\n' || mountPoint->back() == '.')) mountPoint->pop_back();
    }
    return {};
}

Outcome unmount(const std::string& device)
{
    const auto result = Process::run({"udisksctl", "unmount", "-b", device});
    if (!result.started) return {false, "udisksctl is not installed."};
    if (result.exitCode != 0) return {false, lastLine(result.errors)};
    return {};
}

} // namespace Drives
