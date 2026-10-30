#include "properties/properties.h"

#include "btrfs/btrfsinfo.h"
#include "diskspace/diskspace.h"
#include "entry/entry.h"
#include "fileflags/fileflags.h"
#include "filetype/filetype.h"
#include "format/format.h"
#include "mounts/mounts.h"
#include "pathbar/pathbar.h"
#include "permissions/permissions.h"
#include "xattrs/xattrs.h"

#include <cerrno>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <optional>
#include <set>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

namespace {

constexpr int kMaxLinkChain = 40;

std::optional<struct statx> readStat(const std::string& path)
{
    struct statx st {};
    const unsigned mask = STATX_BASIC_STATS | STATX_BTIME | STATX_MNT_ID;
    if (statx(AT_FDCWD, path.c_str(), AT_SYMLINK_NOFOLLOW | AT_NO_AUTOMOUNT, mask, &st) != 0) return std::nullopt;
    return st;
}

std::string permissionWords(std::uint32_t bits)
{
    std::string words;
    if (bits & 4) words = "Read";
    if (bits & 2) words += words.empty() ? "Write" : ", write";
    if (bits & 1) words += words.empty() ? "Execute" : ", execute";
    return words.empty() ? "None" : words;
}

std::string dateRow(const struct statx_timestamp& when)
{
    return Format::fullDate(when.tv_sec, when.tv_nsec);
}

std::string typeNameOf(const std::string& path)
{
    const int fd = open(PathBar::parent(path).c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) return {};
    Entry entry;
    const bool ok = readEntry(fd, PathBar::parent(path), PathBar::baseName(path), entry);
    close(fd);
    if (!ok) return {};
    detectFileType(entry);
    return entry.typeName;
}

PropertySection generalSection(const std::string& path, const struct statx& st)
{
    PropertySection section {"General", {}};
    auto& rows = section.rows;
    rows.push_back({"Name", path == "/" ? "/" : PathBar::baseName(path)});
    rows.push_back({"Type", typeNameOf(path)});
    rows.push_back({"Location", PathBar::parent(path)});
    rows.push_back({"Full path", path});
    char real[PATH_MAX];
    if (realpath(path.c_str(), real) && path != real) rows.push_back({"Real path", real});
    rows.push_back({"Size", Format::exactSize(st.stx_size)});
    const std::uint64_t onDisk = st.stx_blocks * 512;
    rows.push_back({"Size on disk", Format::exactSize(onDisk)});
    if (S_ISREG(st.stx_mode) && onDisk < st.stx_size) rows.push_back({"Sparse", "Yes, or compressed: it uses less disk than its size"});
    if (st.stx_mask & STATX_BTIME) rows.push_back({"Created", dateRow(st.stx_btime)});
    else rows.push_back({"Created", "This filesystem does not record it"});
    rows.push_back({"Modified", dateRow(st.stx_mtime)});
    rows.push_back({"Metadata changed", dateRow(st.stx_ctime)});
    rows.push_back({"Accessed", dateRow(st.stx_atime)});
    return section;
}

PropertySection permissionsSection(const std::string& path, const struct statx& st)
{
    PropertySection section {"Permissions", {}};
    auto& rows = section.rows;
    const std::uint32_t mode = st.stx_mode;
    rows.push_back({"Owner", Permissions::userName(st.stx_uid) + " (" + std::to_string(st.stx_uid) + ")"});
    rows.push_back({"Group", Permissions::groupName(st.stx_gid) + " (" + std::to_string(st.stx_gid) + ")"});
    rows.push_back({"Mode", Permissions::modeText(mode)});
    rows.push_back({"Octal", Permissions::octalText(mode)});
    rows.push_back({"Owner can", permissionWords(mode >> 6)});
    rows.push_back({"Group can", permissionWords(mode >> 3)});
    rows.push_back({"Others can", permissionWords(mode)});
    rows.push_back({"Set user ID", (mode & S_ISUID) ? "On" : "Off"});
    rows.push_back({"Set group ID", (mode & S_ISGID) ? "On" : "Off"});
    rows.push_back({"Sticky bit", (mode & S_ISVTX) ? "On" : "Off"});
    rows.push_back({"You can", Permissions::whatYouCanDo(path, S_ISDIR(mode))});

    const auto aclRows = [&rows](const char* label, const std::vector<std::string>& lines) {
        std::string text;
        for (const std::string& line : lines) text += (text.empty() ? "" : "\n") + line;
        rows.push_back({label, text.empty() ? "None" : text});
    };
    aclRows("Access ACL", Permissions::parseAcl(Xattrs::get(path, "system.posix_acl_access")));
    if (S_ISDIR(mode)) aclRows("Default ACL", Permissions::parseAcl(Xattrs::get(path, "system.posix_acl_default")));

    const std::string capabilities = Permissions::parseCapabilities(Xattrs::get(path, "security.capability"));
    rows.push_back({"Capabilities", capabilities.empty() ? "None" : capabilities});
    return section;
}

std::string statxAttributes(const struct statx& st)
{
    const struct { std::uint64_t bit; const char* name; } names[] = {
        {STATX_ATTR_COMPRESSED, "compressed"}, {STATX_ATTR_IMMUTABLE, "immutable"}, {STATX_ATTR_APPEND, "append only"},
        {STATX_ATTR_NODUMP, "no dump"}, {STATX_ATTR_ENCRYPTED, "encrypted"}, {STATX_ATTR_AUTOMOUNT, "automount"},
        {STATX_ATTR_MOUNT_ROOT, "mount root"}, {STATX_ATTR_VERITY, "verity"}, {STATX_ATTR_DAX, "direct access"},
    };
    std::string text;
    for (const auto& entry : names)
        if (st.stx_attributes & st.stx_attributes_mask & entry.bit)
            text += (text.empty() ? "" : ", ") + std::string(entry.name);
    return text.empty() ? "None" : text;
}

PropertySection filesystemSection(const std::string& path, const struct statx& st, const Mount& mount)
{
    PropertySection section {"Filesystem", {}};
    auto& rows = section.rows;
    rows.push_back({"Inode", std::to_string(st.stx_ino)});
    rows.push_back({"Device", std::to_string(st.stx_dev_major) + ":" + std::to_string(st.stx_dev_minor)});
    if (S_ISCHR(st.stx_mode) || S_ISBLK(st.stx_mode))
        rows.push_back({"Device number", std::to_string(st.stx_rdev_major) + ":" + std::to_string(st.stx_rdev_minor)});
    rows.push_back({"Hard links", std::to_string(st.stx_nlink)});
    rows.push_back({"Block size", Format::number(st.stx_blksize) + " bytes"});
    rows.push_back({"Blocks", Format::number(st.stx_blocks) + " (512 bytes each)"});
    if (!mount.mountPoint.empty()) {
        rows.push_back({"Filesystem type", mount.type});
        rows.push_back({"Source device", mount.source});
        rows.push_back({"Mount point", mount.mountPoint});
        rows.push_back({"Mounted part", mount.root});
        rows.push_back({"Mount options", mount.options});
        rows.push_back({"Filesystem options", mount.superOptions});
    }
    if (st.stx_mask & STATX_MNT_ID) rows.push_back({"Mount ID", std::to_string(st.stx_mnt_id)});
    const DiskSpace disk = readDiskSpace(S_ISLNK(st.stx_mode) ? PathBar::parent(path) : path);
    if (disk.known) {
        rows.push_back({"Disk size", Format::exactSize(disk.total)});
        rows.push_back({"Disk free", Format::exactSize(disk.available)});
    }
    const FileFlags::Flags flags = FileFlags::read(path);
    if (flags.known) {
        rows.push_back({"File flags", flags.letters});
        rows.push_back({"Flags that are on", flags.names.empty() ? "None" : flags.names});
    }
    rows.push_back({"Kernel attributes", statxAttributes(st)});
    return section;
}

PropertySection attributesSection(const std::string& path)
{
    PropertySection section {"Attributes", {}};
    for (const Xattr& attribute : Xattrs::read(path))
        section.rows.push_back({attribute.name, Xattrs::display(attribute.value)});
    if (section.rows.empty()) section.rows.push_back({"Extended attributes", "None"});
    return section;
}

PropertySection btrfsSection(const std::string& path, const BtrfsInfo& info)
{
    PropertySection section {"Btrfs", {}};
    auto& rows = section.rows;
    rows.push_back({"Subvolume ID", std::to_string(info.subvolumeId)});
    rows.push_back({"Compression setting", info.compressionProperty.empty() ? "Inherited" : info.compressionProperty});
    struct stat st {};
    if (lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
        rows.push_back({"Extents", Format::number(info.extents)});
        rows.push_back({"Compressed extents", Format::number(info.compressedExtents)});
        rows.push_back({"Data in compressed extents", Format::exactSize(info.compressedLogicalBytes)});
        rows.push_back({"Shared space", Format::exactSize(info.sharedBytes)});
        rows.push_back({"Exclusive space", Format::exactSize(info.exclusiveBytes)});
        rows.push_back({"Compression ratio", "Only root can read this. Run: sudo compsize \"" + path + "\""});
    }
    return section;
}

PropertySection linkSection(const std::string& path)
{
    PropertySection section {"Link", {}};
    auto& rows = section.rows;

    std::string chain, current = path;
    bool broken = false;
    for (int hop = 0; hop < kMaxLinkChain; ++hop) {
        char target[PATH_MAX];
        const ssize_t size = readlink(current.c_str(), target, sizeof target - 1);
        if (size < 0) break;
        target[size] = '\0';
        if (hop == 0) rows.push_back({"Points to", target});
        current = target[0] == '/' ? target : PathBar::normalize(PathBar::parent(current) + "/" + target);
        chain += (chain.empty() ? "" : "\n") + current;
        struct stat st {};
        if (lstat(current.c_str(), &st) != 0) {
            broken = true;
            break;
        }
    }
    char real[PATH_MAX];
    if (!broken && realpath(path.c_str(), real)) rows.push_back({"Final target", real});
    rows.push_back({"Status", broken ? "Broken: the target does not exist" : "Working"});
    rows.push_back({"Link chain", chain});
    return section;
}

} // namespace

namespace Properties {

std::vector<PropertySection> collect(const std::string& path)
{
    const auto st = readStat(path);
    if (!st) return {{"General", {{"Error", std::string("Can't read this item: ") + std::strerror(errno)}}}};

    const Mount mount = (st->stx_mask & STATX_MNT_ID) ? Mounts::find(st->stx_mnt_id) : Mount {};
    std::vector<PropertySection> sections {generalSection(path, *st), permissionsSection(path, *st),
                                           filesystemSection(path, *st, mount), attributesSection(path)};
    if (S_ISLNK(st->stx_mode)) sections.push_back(linkSection(path));
    if (mount.type == "btrfs") {
        const BtrfsInfo info = readBtrfsInfo(path);
        if (info.known) sections.push_back(btrfsSection(path, info));
    }
    return sections;
}

std::vector<PropertySection> collectSelection(const std::vector<std::string>& paths)
{
    std::uint64_t files = 0, folders = 0, links = 0, others = 0, size = 0, disk = 0;
    std::set<std::string> owners, groups, modes, parents, types;
    for (const std::string& path : paths) {
        const auto st = readStat(path);
        if (!st) continue;
        if (S_ISREG(st->stx_mode)) ++files;
        else if (S_ISDIR(st->stx_mode)) ++folders;
        else if (S_ISLNK(st->stx_mode)) ++links;
        else ++others;
        if (S_ISREG(st->stx_mode)) size += st->stx_size;
        disk += st->stx_blocks * 512;
        owners.insert(Permissions::userName(st->stx_uid));
        groups.insert(Permissions::groupName(st->stx_gid));
        modes.insert(Permissions::modeText(st->stx_mode));
        parents.insert(PathBar::parent(path));
        types.insert(typeNameOf(path));
    }

    const auto shared = [](const std::set<std::string>& values) {
        return values.size() == 1 ? *values.begin() : "Mixed (" + std::to_string(values.size()) + " different)";
    };

    PropertySection totals {"General", {
        {"Selected", Format::itemCount(paths.size())},
        {"Files", Format::number(files)},
        {"Folders", Format::number(folders)},
        {"Links", Format::number(links)},
        {"Other items", Format::number(others)},
        {"Size of files", Format::exactSize(size)},
        {"Size on disk", Format::exactSize(disk)},
        {"Location", shared(parents)},
        {"Type", shared(types)},
        {"Owner", shared(owners)},
        {"Group", shared(groups)},
        {"Mode", shared(modes)},
    }};
    return {totals};
}

PropertySection folderContents(const FolderScan& scan)
{
    PropertySection section {"Contents", {
        {"Total size", Format::exactSize(scan.totalSize)},
        {"Size on disk", Format::exactSize(scan.diskUsage)},
        {"Files", Format::number(scan.files)},
        {"Folders", Format::number(scan.folders)},
        {"Links", Format::number(scan.links)},
        {"Other items", Format::number(scan.others)},
        {"Hidden items", Format::number(scan.hidden)},
        {"Folders that can't be read", Format::number(scan.unreadable)},
    }};
    std::string largest;
    for (const auto& [size, path] : scan.largest)
        largest += (largest.empty() ? "" : "\n") + Format::size(size) + "  " + path;
    if (!largest.empty()) section.rows.push_back({"Largest files", largest});
    return section;
}

} // namespace Properties
