#include "permissions/permissions.h"

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

namespace {

// the on-disk formats, from linux/posix_acl_xattr.h and linux/capability.h
constexpr std::uint16_t kAclUserObj = 0x01, kAclUser = 0x02, kAclGroupObj = 0x04, kAclGroup = 0x08,
                        kAclMask = 0x10, kAclOther = 0x20;
constexpr std::uint32_t kCapEffective = 0x000001;

std::uint16_t read16(const std::string& data, std::size_t at)
{
    std::uint16_t value = 0;
    std::memcpy(&value, data.data() + at, sizeof value);
    return value;
}

std::uint32_t read32(const std::string& data, std::size_t at)
{
    std::uint32_t value = 0;
    std::memcpy(&value, data.data() + at, sizeof value);
    return value;
}

std::string rwx(std::uint32_t bits)
{
    std::string text = "---";
    if (bits & 4) text[0] = 'r';
    if (bits & 2) text[1] = 'w';
    if (bits & 1) text[2] = 'x';
    return text;
}

const char* const kCapabilityNames[] = {
    "cap_chown", "cap_dac_override", "cap_dac_read_search", "cap_fowner", "cap_fsetid", "cap_kill", "cap_setgid",
    "cap_setuid", "cap_setpcap", "cap_linux_immutable", "cap_net_bind_service", "cap_net_broadcast", "cap_net_admin",
    "cap_net_raw", "cap_ipc_lock", "cap_ipc_owner", "cap_sys_module", "cap_sys_rawio", "cap_sys_chroot", "cap_sys_ptrace",
    "cap_sys_pacct", "cap_sys_admin", "cap_sys_boot", "cap_sys_nice", "cap_sys_resource", "cap_sys_time",
    "cap_sys_tty_config", "cap_mknod", "cap_lease", "cap_audit_write", "cap_audit_control", "cap_setfcap",
    "cap_mac_override", "cap_mac_admin", "cap_syslog", "cap_wake_alarm", "cap_block_suspend", "cap_audit_read",
    "cap_perfmon", "cap_bpf", "cap_checkpoint_restore",
};

} // namespace

namespace Permissions {

std::string userName(std::uint32_t uid)
{
    const passwd* user = getpwuid(uid);
    return user && user->pw_name ? user->pw_name : "unknown";
}

std::string groupName(std::uint32_t gid)
{
    const group* entry = getgrgid(gid);
    return entry && entry->gr_name ? entry->gr_name : "unknown";
}

std::string modeText(std::uint32_t mode)
{
    char type = '?';
    if (S_ISREG(mode)) type = '-';
    else if (S_ISDIR(mode)) type = 'd';
    else if (S_ISLNK(mode)) type = 'l';
    else if (S_ISCHR(mode)) type = 'c';
    else if (S_ISBLK(mode)) type = 'b';
    else if (S_ISFIFO(mode)) type = 'p';
    else if (S_ISSOCK(mode)) type = 's';

    std::string text = type + rwx(mode >> 6) + rwx(mode >> 3) + rwx(mode);
    if (mode & S_ISUID) text[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID) text[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX) text[9] = (mode & S_IXOTH) ? 't' : 'T';
    return text;
}

std::string octalText(std::uint32_t mode)
{
    char text[8];
    std::snprintf(text, sizeof text, "%04o", mode & 07777);
    return text;
}

std::string whatYouCanDo(const std::string& path, bool isDirectory)
{
    const auto can = [&path](int how) { return faccessat(AT_FDCWD, path.c_str(), how, AT_EACCESS) == 0; };
    std::string text;
    const auto add = [&text](const char* word) { text += text.empty() ? word : std::string(", ") + word; };
    if (can(R_OK)) add(isDirectory ? "list contents" : "read");
    if (can(W_OK)) add(isDirectory ? "add and remove items" : "write");
    if (can(X_OK)) add(isDirectory ? "enter" : "run");
    return text.empty() ? "nothing" : text;
}

std::vector<std::string> parseAcl(const std::string& value)
{
    std::vector<std::string> lines;
    constexpr std::size_t kHeader = 4, kEntry = 8;
    if (value.size() < kHeader) return lines;

    for (std::size_t at = kHeader; at + kEntry <= value.size(); at += kEntry) {
        const std::uint16_t tag = read16(value, at);
        const std::uint32_t bits = read16(value, at + 2);
        const std::uint32_t id = read32(value, at + 4);
        std::string line;
        switch (tag) {
        case kAclUserObj: line = "user::"; break;
        case kAclUser: line = "user:" + userName(id) + ":"; break;
        case kAclGroupObj: line = "group::"; break;
        case kAclGroup: line = "group:" + groupName(id) + ":"; break;
        case kAclMask: line = "mask::"; break;
        case kAclOther: line = "other::"; break;
        default: continue;
        }
        lines.push_back(line + rwx(bits));
    }
    return lines;
}

std::string parseCapabilities(const std::string& value)
{
    if (value.size() < 12) return {};
    const std::uint32_t magic = read32(value, 0);
    const std::uint64_t permitted = read32(value, 4)
        | (value.size() >= 16 ? static_cast<std::uint64_t>(read32(value, 12)) << 32 : 0);
    const std::uint64_t inheritable = read32(value, 8)
        | (value.size() >= 20 ? static_cast<std::uint64_t>(read32(value, 16)) << 32 : 0);
    const bool effective = (magic & kCapEffective) != 0;

    std::string text;
    constexpr int count = static_cast<int>(sizeof kCapabilityNames / sizeof kCapabilityNames[0]);
    for (int i = 0; i < count; ++i) {
        const bool p = (permitted >> i) & 1, in = (inheritable >> i) & 1;
        if (!p && !in) continue;
        if (!text.empty()) text += " ";
        text += kCapabilityNames[i];
        text += "=";
        if (effective) text += "e";
        if (in) text += "i";
        if (p) text += "p";
    }
    return text;
}

} // namespace Permissions
