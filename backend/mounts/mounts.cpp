#include "mounts/mounts.h"

#include <fstream>
#include <sstream>

namespace Mounts {

std::string unescape(const std::string& text)
{
    std::string out;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\\' && i + 3 < text.size()) {
            out += static_cast<char>((text[i + 1] - '0') * 64 + (text[i + 2] - '0') * 8 + (text[i + 3] - '0'));
            i += 3;
        } else {
            out += text[i];
        }
    }
    return out;
}

Mount parseLine(const std::string& line)
{
    // 36 25 0:32 /root / rw,noatime shared:1 - btrfs /dev/sda3 rw,compress=zstd:1
    Mount mount;
    const auto separator = line.find(" - ");
    if (separator == std::string::npos) return mount;

    std::istringstream before(line.substr(0, separator));
    std::string parent, device;
    before >> mount.id >> parent >> device >> mount.root >> mount.mountPoint >> mount.options;
    mount.root = unescape(mount.root);
    mount.mountPoint = unescape(mount.mountPoint);

    std::istringstream after(line.substr(separator + 3));
    after >> mount.type >> mount.source >> mount.superOptions;
    mount.source = unescape(mount.source);
    return mount;
}

std::vector<Mount> all()
{
    std::vector<Mount> mounts;
    std::ifstream file("/proc/self/mountinfo");
    std::string line;
    while (std::getline(file, line)) {
        Mount mount = parseLine(line);
        if (!mount.mountPoint.empty()) mounts.push_back(std::move(mount));
    }
    return mounts;
}

Mount find(std::uint64_t id)
{
    for (Mount& mount : all())
        if (mount.id == id) return mount;
    return {};
}

} // namespace Mounts
