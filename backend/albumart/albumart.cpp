#include "albumart/albumart.h"

#include "pathbar/pathbar.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <system_error>
#include <unistd.h>

namespace {

constexpr std::size_t kMaxPicture = 64 * 1024 * 1024;   // a bigger "picture" means a damaged file
constexpr unsigned char kFrontCover = 3;                // the picture type both formats use
constexpr int kMaxFlacBlocks = 1024;                    // more than any real file: a damaged one could go on and on

class File {
public:
    explicit File(const std::string& path) : m_fd(open(path.c_str(), O_RDONLY | O_CLOEXEC)) {}
    ~File() { if (m_fd >= 0) close(m_fd); }
    File(const File&) = delete;
    File& operator=(const File&) = delete;

    bool ok() const { return m_fd >= 0; }
    bool readAt(std::uint64_t offset, void* out, std::size_t size) const
    {
        auto* bytes = static_cast<char*>(out);
        while (size > 0) {
            const ssize_t got = pread(m_fd, bytes, size, static_cast<off_t>(offset));
            if (got <= 0) return false;
            bytes += got;
            offset += static_cast<std::uint64_t>(got);
            size -= static_cast<std::size_t>(got);
        }
        return true;
    }

private:
    int m_fd;
};

std::uint32_t bigEndian(const unsigned char* bytes, int count)
{
    std::uint32_t value = 0;
    for (int i = 0; i < count; ++i) value = (value << 8) | bytes[i];
    return value;
}

// each byte holds 7 bits, so that no size can look like an MP3 frame
std::uint32_t syncSafe(const unsigned char* bytes)
{
    return (std::uint32_t(bytes[0] & 0x7f) << 21) | (std::uint32_t(bytes[1] & 0x7f) << 14)
         | (std::uint32_t(bytes[2] & 0x7f) << 7) | (bytes[3] & 0x7f);
}

// A FLAC PICTURE block: type, then four length-prefixed or fixed fields, then the image.
bool flacPicture(const std::string& block, unsigned char* type, std::string* picture)
{
    const auto* bytes = reinterpret_cast<const unsigned char*>(block.data());
    std::size_t at = 0;
    const auto number = [&](std::uint32_t* value) {
        if (block.size() - at < 4) return false;
        *value = bigEndian(bytes + at, 4);
        at += 4;
        return true;
    };
    const auto skipText = [&] {
        std::uint32_t length = 0;
        if (!number(&length) || length > block.size() - at) return false;
        at += length;
        return true;
    };

    std::uint32_t pictureType = 0, length = 0, ignored = 0;
    if (!number(&pictureType) || !skipText() || !skipText()) return false;
    for (int i = 0; i < 4; ++i)                               // width, height, depth, colours
        if (!number(&ignored)) return false;
    if (!number(&length) || length == 0 || length > block.size() - at) return false;
    *type = static_cast<unsigned char>(pictureType);
    *picture = block.substr(at, length);
    return true;
}

AlbumArt::Found readFlac(const File& file, std::uint64_t at)
{
    AlbumArt::Found found;
    at += 4;                                                  // "fLaC"
    for (int block = 0;; ++block) {
        unsigned char header[4];
        if (block == kMaxFlacBlocks || !file.readAt(at, header, sizeof header)) return found;   // ffmpeg can try
        const bool last = header[0] & 0x80;
        const int kind = header[0] & 0x7f;
        const std::uint32_t size = bigEndian(header + 1, 3);
        at += sizeof header;

        constexpr int kPictureBlock = 6;
        if (kind == kPictureBlock && size <= kMaxPicture) {
            std::string block(size, '\0');
            if (!file.readAt(at, block.data(), size)) return found;
            unsigned char type = 0;
            std::string picture;
            if (flacPicture(block, &type, &picture) && (found.picture.empty() || type == kFrontCover))
                found.picture = std::move(picture);
            if (type == kFrontCover && !found.picture.empty()) break;
        }
        at += size;
        if (last) break;
    }
    found.understood = true;
    return found;
}

// A frame's content: encoding, MIME type, picture type, description, then the image.
bool id3Picture(const unsigned char* frame, std::size_t size, bool oldFormat, unsigned char* type, std::string* picture)
{
    if (size < 2) return false;
    const unsigned char encoding = frame[0];
    std::size_t at = 1;
    if (oldFormat) {                                          // ID3v2.2: three letters, "JPG"
        at += 3;
    } else {
        while (at < size && frame[at] != 0) ++at;
        ++at;
    }
    if (at >= size) return false;
    *type = frame[at++];

    const bool wide = encoding == 1 || encoding == 2;         // UTF-16 ends its text with two zero bytes
    if (wide) {
        while (at + 1 < size && (frame[at] != 0 || frame[at + 1] != 0)) at += 2;
        at += 2;
    } else {
        while (at < size && frame[at] != 0) ++at;
        ++at;
    }
    if (at >= size) return false;
    picture->assign(reinterpret_cast<const char*>(frame + at), size - at);
    return true;
}

AlbumArt::Found readId3(const File& file, const unsigned char* header, std::uint32_t tagSize)
{
    AlbumArt::Found found;
    const int version = header[3];
    const unsigned char flags = header[5];
    constexpr int kHeaderSize = 10;
    constexpr unsigned char kWholeTagUnsynchronised = 0x80, kExtendedHeader = 0x40;
    if (version < 2 || version > 4 || (flags & kWholeTagUnsynchronised) || tagSize > kMaxPicture) return found;

    std::string tag(tagSize, '\0');
    if (!file.readAt(kHeaderSize, tag.data(), tagSize)) return found;
    const auto* bytes = reinterpret_cast<const unsigned char*>(tag.data());

    std::size_t at = 0;
    if ((flags & kExtendedHeader) && version >= 3) {
        if (tagSize < 4) return found;
        at = version == 4 ? syncSafe(bytes) : bigEndian(bytes, 4) + 4;
        if (at > tagSize) return found;
    }

    const bool oldFormat = version == 2;
    const std::size_t idSize = oldFormat ? 3 : 4;
    const std::size_t frameHeader = oldFormat ? 6 : 10;
    while (tagSize - at >= frameHeader && bytes[at] != 0) {           // zero bytes are the padding after the last frame
        const std::uint32_t size = oldFormat ? bigEndian(bytes + at + 3, 3)
                                 : version == 4 ? syncSafe(bytes + at + 4) : bigEndian(bytes + at + 4, 4);
        const std::size_t content = at + frameHeader;
        if (size > tagSize - content) return found;                   // doesn't fit: not to be trusted

        if (std::memcmp(bytes + at, oldFormat ? "PIC" : "APIC", idSize) == 0) {
            // v2.3 and v2.4 keep a frame's compression, encryption and the like in its second flag byte
            if (!oldFormat && bytes[at + 9] != 0) return found;
            unsigned char type = 0;
            std::string picture;
            if (id3Picture(bytes + content, size, oldFormat, &type, &picture) && (found.picture.empty() || type == kFrontCover))
                found.picture = std::move(picture);
            if (type == kFrontCover && !found.picture.empty()) break;
        }
        at = content + size;
    }
    found.understood = true;
    return found;
}

} // namespace

namespace AlbumArt {

Found read(const std::string& path)
{
    const File file(path);
    unsigned char header[10];
    if (!file.ok() || !file.readAt(0, header, sizeof header)) return {};

    if (std::memcmp(header, "fLaC", 4) == 0) return readFlac(file, 0);
    if (std::memcmp(header, "ID3", 3) == 0) {
        const std::uint32_t tagSize = syncSafe(header + 6);
        unsigned char after[4];
        if (file.readAt(sizeof header + tagSize, after, sizeof after) && std::memcmp(after, "fLaC", 4) == 0) {
            Found found = readFlac(file, sizeof header + tagSize);    // an ID3 tag put in front of a FLAC file
            if (!found.picture.empty()) return found;
        }
        return readId3(file, header, tagSize);
    }
    const bool mpegAudio = header[0] == 0xff && (header[1] & 0xe0) == 0xe0;
    if (mpegAudio) {                                                  // an MP3 without a tag: there is no picture to find
        Found none;
        none.understood = true;
        return none;
    }
    return {};
}

std::string coverBeside(const std::string& path)
{
    static const char* const names[] = {"cover.jpg", "cover.jpeg", "cover.png", "folder.jpg", "folder.jpeg", "folder.png",
                                        "front.jpg", "front.jpeg", "front.png"};
    std::string found;
    auto rank = std::end(names);
    std::error_code error;
    for (const auto& item : std::filesystem::directory_iterator(PathBar::parent(path), error)) {
        std::string name = item.path().filename().string();
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
        const auto match = std::find(std::begin(names), rank, name);
        if (match == rank) continue;
        rank = match;                                  // the list above is also the order of preference
        found = item.path().string();
    }
    return found;
}

} // namespace AlbumArt
