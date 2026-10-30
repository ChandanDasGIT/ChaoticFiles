#pragma once
#include <cstdint>
#include <string>

enum class EntryKind { File, Directory, Socket, Pipe, CharDevice, BlockDevice, Unknown };

enum class FileCategory {
    Folder, Text, Code, Image, Audio, Video, Archive, Pdf, Document, Spreadsheet,
    Presentation, Program, DiskImage, Font, System, Unknown
};

// One file or folder, as read from the disk. A link takes its kind and size from what it points to.
struct Entry {
    std::string name;
    std::string path;
    std::string typeName;                        // "PNG Image"; filled in by detectFileType
    EntryKind kind = EntryKind::Unknown;
    FileCategory category = FileCategory::Unknown;
    std::uint64_t size = 0;
    std::int64_t modified = 0;                   // seconds since 1970
    std::uint32_t modifiedNanos = 0;
    std::uint32_t mode = 0;                      // permission bits
    bool hidden = false;
    bool symlink = false;
    bool brokenLink = false;

    bool isDirectory() const { return kind == EntryKind::Directory; }
};

// Reads one name inside the open folder dirFd. On failure returns false and leaves errno set.
bool readEntry(int dirFd, const std::string& dirPath, const std::string& name, Entry& out);

// Same file on disk as far as a folder listing can tell.
bool sameOnDisk(const Entry& a, const Entry& b);
