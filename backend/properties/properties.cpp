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

#include <algorithm>
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

const char* const kCounting = "Counting…";
const char* const kAdvanced = "Advanced";

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

using Rows = std::vector<PropertyRow>;

PropertySection generalSection(const std::string& path, const struct statx& st, Rows& advanced)
{
    PropertySection section {"General", {}};
    auto& rows = section.rows;
    rows.push_back({"Name", path == "/" ? "/" : PathBar::baseName(path)});
    rows.push_back({"Type", typeNameOf(path)});
    rows.push_back({"Location", PathBar::parent(path)});
    char real[PATH_MAX];
    if (realpath(path.c_str(), real) && path != real) rows.push_back({"Real path", real});
    const std::uint64_t onDisk = st.stx_blocks * 512;
    if (S_ISDIR(st.stx_mode)) {                      // filled in by applyTotals as the folder is scanned
        rows.push_back({"Size", kCounting});
        rows.push_back({"Size on disk", kCounting});
    } else {
        rows.push_back({"Size", Format::exactSize(st.stx_size)});
        rows.push_back({"Size on disk", Format::exactSize(onDisk)});
    }
    if (S_ISREG(st.stx_mode) && onDisk < st.stx_size) rows.push_back({"Note", "Takes less disk space than its size: it is sparse or compressed"});
    if (st.stx_mask & STATX_BTIME) rows.push_back({"Created", Format::readableDate(st.stx_btime.tv_sec)});
    rows.push_back({"Modified", Format::readableDate(st.stx_mtime.tv_sec)});
    rows.push_back({"Accessed", Format::readableDate(st.stx_atime.tv_sec)});

    advanced.push_back({"Full path", path});
    advanced.push_back({"Created (exact)", (st.stx_mask & STATX_BTIME) ? dateRow(st.stx_btime) : "This filesystem does not record it"});
    advanced.push_back({"Modified (exact)", dateRow(st.stx_mtime)});
    advanced.push_back({"Metadata changed", dateRow(st.stx_ctime)});
    advanced.push_back({"Accessed (exact)", dateRow(st.stx_atime)});
    return section;
}

PropertySection permissionsSection(const std::string& path, const struct statx& st, Rows& advanced)
{
    PropertySection section {"Permissions", {}};
    auto& rows = section.rows;
    const std::uint32_t mode = st.stx_mode;
    const bool folder = S_ISDIR(mode);
    rows.push_back({"Owner", Permissions::userName(st.stx_uid)});
    rows.push_back({"Group", Permissions::groupName(st.stx_gid)});
    rows.push_back({"Owner can", permissionWords(mode >> 6)});
    rows.push_back({"Group can", permissionWords(mode >> 3)});
    rows.push_back({"Others can", permissionWords(mode)});
    rows.push_back({"You can", Permissions::whatYouCanDo(path, folder)});
    if (mode & S_ISUID) rows.push_back({"Runs as owner", "On: it runs with its owner's rights, whoever starts it"});
    if (mode & S_ISGID)
        rows.push_back({folder ? "Shared group" : "Runs as group",
                        folder ? "On: new items inside get this folder's group" : "On: it runs with its group's rights, whoever starts it"});
    if (mode & S_ISVTX) rows.push_back({"Restricted deletion", "On: only an item's owner can delete or rename it in here"});

    const auto joined = [](const std::vector<std::string>& lines) {
        std::string text;
        for (const std::string& line : lines) text += (text.empty() ? "" : "\n") + line;
        return text;
    };
    const std::string access = joined(Permissions::parseAcl(Xattrs::get(path, "system.posix_acl_access")));
    if (!access.empty()) rows.push_back({"Extra rules (ACL)", access});
    const std::string defaults = folder ? joined(Permissions::parseAcl(Xattrs::get(path, "system.posix_acl_default"))) : "";
    if (!defaults.empty()) rows.push_back({"Rules for new items (ACL)", defaults});
    const std::string capabilities = Permissions::parseCapabilities(Xattrs::get(path, "security.capability"));
    if (!capabilities.empty()) rows.push_back({"Special rights", capabilities});

    advanced.push_back({"Mode", Permissions::modeText(mode)});
    advanced.push_back({"Octal", Permissions::octalText(mode)});
    advanced.push_back({"Owner ID", std::to_string(st.stx_uid)});
    advanced.push_back({"Group ID", std::to_string(st.stx_gid)});
    advanced.push_back({"Set user ID", (mode & S_ISUID) ? "On" : "Off"});
    advanced.push_back({"Set group ID", (mode & S_ISGID) ? "On" : "Off"});
    advanced.push_back({"Sticky bit", (mode & S_ISVTX) ? "On" : "Off"});
    if (access.empty()) advanced.push_back({"Access ACL", "None"});
    if (folder && defaults.empty()) advanced.push_back({"Default ACL", "None"});
    if (capabilities.empty()) advanced.push_back({"Capabilities", "None"});
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
    return text;
}

// flags a person would want to know about; "extents" and the like are how the disk stores every file
std::string notableFlags(const std::string& names)
{
    static const char* const notable[] = {"immutable", "append only", "compressed", "encrypted", "no copy on write",
                                          "case insensitive", "verity", "no dump", "no access time"};
    std::string text;
    for (const char* name : notable)
        if ((", " + names + ",").find(", " + std::string(name) + ",") != std::string::npos)
            text += (text.empty() ? "" : ", ") + std::string(name);
    return text;
}

PropertySection diskSection(const std::string& path, const struct statx& st, const Mount& mount, Rows& advanced)
{
    PropertySection section {"Disk", {}};
    auto& rows = section.rows;
    if (!mount.mountPoint.empty()) {
        rows.push_back({"Disk", mount.source});
        rows.push_back({"Filesystem", mount.type});
        rows.push_back({"Mounted at", mount.mountPoint});
    }
    const DiskSpace disk = readDiskSpace(S_ISLNK(st.stx_mode) ? PathBar::parent(path) : path);
    if (disk.known) rows.push_back({"Free space", Format::size(disk.available) + " free of " + Format::size(disk.total)});
    if (!S_ISDIR(st.stx_mode) && st.stx_nlink > 1)
        rows.push_back({"Other names", "This file also has " + std::to_string(st.stx_nlink - 1) + " other name(s) (hard links)"});
    const FileFlags::Flags flags = FileFlags::read(path);
    const std::string notable = flags.known ? notableFlags(flags.names) : "";
    if (!notable.empty()) rows.push_back({"Special flags", notable});
    const std::string attributes = statxAttributes(st);

    advanced.push_back({"Inode", std::to_string(st.stx_ino)});
    advanced.push_back({"Device", std::to_string(st.stx_dev_major) + ":" + std::to_string(st.stx_dev_minor)});
    if (S_ISCHR(st.stx_mode) || S_ISBLK(st.stx_mode))
        advanced.push_back({"Device number", std::to_string(st.stx_rdev_major) + ":" + std::to_string(st.stx_rdev_minor)});
    advanced.push_back({"Hard links", std::to_string(st.stx_nlink)});
    advanced.push_back({"Block size", Format::number(st.stx_blksize) + " bytes"});
    advanced.push_back({"Blocks", Format::number(st.stx_blocks) + " (512 bytes each)"});
    if (!mount.mountPoint.empty()) {
        advanced.push_back({"Mounted part", mount.root});
        advanced.push_back({"Mount options", mount.options});
        advanced.push_back({"Filesystem options", mount.superOptions});
    }
    if (st.stx_mask & STATX_MNT_ID) advanced.push_back({"Mount ID", std::to_string(st.stx_mnt_id)});
    if (disk.known) {
        advanced.push_back({"Disk size", Format::exactSize(disk.total)});
        advanced.push_back({"Disk free", Format::exactSize(disk.available)});
    }
    if (flags.known) {
        advanced.push_back({"File flags", flags.letters});
        advanced.push_back({"Flags that are on", flags.names.empty() ? "None" : flags.names});
    }
    advanced.push_back({"Kernel attributes", attributes.empty() ? "None" : attributes});
    return section;
}

// The attributes desktop apps write, by the names people know them by. The rest is for the Advanced tab.
const char* friendlyAttribute(const std::string& name)
{
    static const struct { const char* key; const char* label; } known[] = {
        {"user.xdg.origin.url", "Downloaded from"}, {"user.xdg.referrer.url", "Download page"},
        {"user.xdg.origin.email.subject", "From email"}, {"user.xdg.origin.email.from", "Email sender"},
        {"user.xdg.comment", "Comment"}, {"user.xdg.tags", "Tags"}, {"user.xdg.language", "Language"},
        {"user.xdg.creator", "Created by"}, {"user.xdg.publisher", "Publisher"},
    };
    for (const auto& entry : known)
        if (name == entry.key) return entry.label;
    return nullptr;
}

PropertySection attributesSection(const std::string& path, Rows& advanced)
{
    PropertySection section {"Attributes", {}};
    bool any = false;
    for (const Xattr& attribute : Xattrs::read(path)) {
        any = true;
        const std::string value = Xattrs::display(attribute.value);
        if (const char* label = friendlyAttribute(attribute.name)) section.rows.push_back({label, value});
        else if (attribute.name == "security.selinux") advanced.push_back({"SELinux label", value});
        else advanced.push_back({"Attribute " + attribute.name, value});
    }
    if (!any) advanced.push_back({"Extended attributes", "None"});
    return section;
}

PropertySection btrfsSection(const std::string& path, const BtrfsInfo& info, Rows& advanced)
{
    PropertySection section {"Btrfs", {}};
    auto& rows = section.rows;
    rows.push_back({"Compression", info.compressionProperty.empty() ? "Same as the rest of the disk" : info.compressionProperty});
    struct stat st {};
    const bool file = lstat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
    if (file) {
        if (info.compressedLogicalBytes > 0)
            rows.push_back({"Compressed data", Format::size(info.compressedLogicalBytes) + " of its " + Format::size(info.logicalBytes)
                                               + " is stored compressed"});
        if (info.sharedBytes > 0)
            rows.push_back({"Shared with copies", Format::size(info.sharedBytes) + ", with a snapshot or a copy that shares its data"});
    }
    advanced.push_back({"Subvolume ID", std::to_string(info.subvolumeId)});
    if (file) {
        advanced.push_back({"Extents", Format::number(info.extents)});
        advanced.push_back({"Compressed extents", Format::number(info.compressedExtents)});
        advanced.push_back({"Data in compressed extents", Format::exactSize(info.compressedLogicalBytes)});
        advanced.push_back({"Shared space", Format::exactSize(info.sharedBytes)});
        advanced.push_back({"Exclusive space", Format::exactSize(info.exclusiveBytes)});
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
    if (chain.find('\n') != std::string::npos) rows.push_back({"Link chain", chain});
    return section;
}

} // namespace

namespace Properties {

std::vector<PropertySection> collect(const std::string& path)
{
    const auto st = readStat(path);
    if (!st) return {{"General", {{"Error", std::string("Can't read this item: ") + std::strerror(errno)}}}};

    const Mount mount = (st->stx_mask & STATX_MNT_ID) ? Mounts::find(st->stx_mnt_id) : Mount {};
    Rows advanced;
    std::vector<PropertySection> sections {generalSection(path, *st, advanced), permissionsSection(path, *st, advanced),
                                           diskSection(path, *st, mount, advanced)};
    PropertySection attributes = attributesSection(path, advanced);
    if (!attributes.rows.empty()) sections.push_back(std::move(attributes));
    if (S_ISLNK(st->stx_mode)) sections.push_back(linkSection(path));
    if (mount.type == "btrfs") {
        const BtrfsInfo info = readBtrfsInfo(path);
        if (info.known) sections.push_back(btrfsSection(path, info, advanced));
    }
    sections.push_back({kAdvanced, std::move(advanced)});
    return sections;
}

void addContent(std::vector<PropertySection>& sections, const std::vector<PropertyRow>& rows)
{
    static const char* const technical[] = {"MIME type", "Detailed type", "Encoding", "Entry point", "Interpreter",
        "Library search path", "Stripped", "Debug information", "Byte order mark",
        "Tagged", "UserProperties", "Suspects", "Form", "JavaScript", "Page rot", "File size", "Optimized",
        "Custom Metadata", "Metadata Stream"};
    PropertySection content {"Content", {}};
    Rows advanced;
    for (const PropertyRow& row : rows) {
        const bool raw = std::find(std::begin(technical), std::end(technical), row.label) != std::end(technical)
                      || row.label.rfind("Linked libraries", 0) == 0 || row.label.find("pixel format") != std::string::npos;
        if (raw) advanced.push_back(row);
        else if (row.label == "CreationDate") content.rows.push_back({"Written", row.value});        // pdfinfo's own names
        else if (row.label == "ModDate") content.rows.push_back({"Last edited", row.value});
        else content.rows.push_back(row);
    }
    auto at = std::find_if(sections.begin(), sections.end(), [](const PropertySection& s) { return s.title == kAdvanced; });
    if (!content.rows.empty()) {
        const auto existing = std::find_if(sections.begin(), sections.end(), [](const PropertySection& s) { return s.title == "Content"; });
        if (existing != sections.end()) *existing = std::move(content);
        else at = sections.insert(at, std::move(content)) + 1;
    }
    if (at == sections.end()) {
        if (!advanced.empty()) sections.push_back({kAdvanced, std::move(advanced)});
        return;
    }
    std::erase_if(at->rows, [](const PropertyRow& row) { return row.label.rfind("Content: ", 0) == 0; });
    for (PropertyRow& row : advanced) at->rows.push_back({"Content: " + row.label, std::move(row.value)});
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

    PropertySection totals {"General", {{"Selected", Format::itemCount(paths.size())}}};
    auto& rows = totals.rows;
    const auto count = [&rows](const char* label, std::uint64_t value) { if (value) rows.push_back({label, Format::number(value)}); };
    count("Files", files);
    count("Folders", folders);
    count("Links", links);
    count("Other items", others);
    rows.push_back({"Size", folders > 0 ? kCounting : Format::exactSize(size)});
    rows.push_back({"Size on disk", folders > 0 ? kCounting : Format::exactSize(disk)});
    rows.push_back({"Location", shared(parents)});
    rows.push_back({"Type", shared(types)});
    rows.push_back({"Owner", shared(owners)});
    rows.push_back({"Group", shared(groups)});
    return {totals, {kAdvanced, {{"Mode", shared(modes)}}}};
}

PropertySection folderContents(const FolderScan& scan)
{
    PropertySection section {"Contents", {}};
    auto& rows = section.rows;
    const auto count = [&rows](const char* label, std::uint64_t value) { if (value) rows.push_back({label, Format::number(value)}); };
    count("Files", scan.files);
    count("Folders", scan.folders);
    count("Links", scan.links);
    count("Other items", scan.others);
    count("Hidden items", scan.hidden);
    count("Folders that can't be read", scan.unreadable);
    if (rows.empty()) rows.push_back({"Contents", scan.finished ? "Empty" : kCounting});
    std::string largest;
    for (const auto& [size, path] : scan.largest)
        largest += (largest.empty() ? "" : "\n") + Format::size(size) + "  " + path;
    if (!largest.empty()) section.rows.push_back({"Largest files", largest});
    return section;
}

void applyTotals(std::vector<PropertySection>& sections, const FolderScan& scan)
{
    const std::string more = scan.finished ? "" : " (counting…)";
    for (PropertySection& section : sections) {
        if (section.title != "General") continue;
        for (PropertyRow& row : section.rows) {
            if (row.label == "Size") row.value = Format::exactSize(scan.totalSize) + more;
            else if (row.label == "Size on disk") row.value = Format::exactSize(scan.diskUsage) + more;
        }
    }
}

} // namespace Properties
