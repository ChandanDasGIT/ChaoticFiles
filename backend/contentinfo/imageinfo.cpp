#include "contentinfo/contentinfo.h"

#include "format/format.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>

namespace {

constexpr std::size_t kReadLimit = 4 * 1024 * 1024;     // EXIF sits at the start of a file

std::uint32_t be32(const unsigned char* p) { return std::uint32_t(p[0]) << 24 | p[1] << 16 | p[2] << 8 | p[3]; }
std::uint16_t be16(const unsigned char* p) { return static_cast<std::uint16_t>(p[0] << 8 | p[1]); }
std::uint32_t le32(const unsigned char* p) { return std::uint32_t(p[3]) << 24 | p[2] << 16 | p[1] << 8 | p[0]; }
std::uint16_t le16(const unsigned char* p) { return static_cast<std::uint16_t>(p[1] << 8 | p[0]); }
std::uint32_t le24(const unsigned char* p) { return std::uint32_t(p[2]) << 16 | p[1] << 8 | p[0]; }

void addSize(std::vector<PropertyRow>& rows, std::uint32_t width, std::uint32_t height)
{
    rows.push_back({"Dimensions", Format::number(width) + " × " + Format::number(height) + " pixels"});
    char megapixels[32];
    std::snprintf(megapixels, sizeof megapixels, "%.1f MP", double(width) * height / 1e6);
    rows.push_back({"Resolution", megapixels});
}

void readPng(const std::string& d, std::vector<PropertyRow>& rows)
{
    const auto* p = reinterpret_cast<const unsigned char*>(d.data());
    if (d.size() < 33) return;
    addSize(rows, be32(p + 16), be32(p + 20));
    static const char* const colours[] = {"Greyscale", "", "RGB", "Indexed", "Greyscale with alpha", "", "RGBA"};
    rows.push_back({"Bit depth", std::to_string(p[24]) + " bits per channel"});
    if (p[25] <= 6) rows.push_back({"Colour type", colours[p[25]]});
    rows.push_back({"Interlaced", p[28] ? "Yes" : "No"});
}

void readGif(const std::string& d, std::vector<PropertyRow>& rows)
{
    const auto* p = reinterpret_cast<const unsigned char*>(d.data());
    if (d.size() >= 10) addSize(rows, le16(p + 6), le16(p + 8));
}

void readBmp(const std::string& d, std::vector<PropertyRow>& rows)
{
    const auto* p = reinterpret_cast<const unsigned char*>(d.data());
    if (d.size() < 30) return;
    addSize(rows, le32(p + 18), le32(p + 22) & 0x7fffffff);
    rows.push_back({"Bit depth", std::to_string(le16(p + 28)) + " bits per pixel"});
}

void readWebp(const std::string& d, std::vector<PropertyRow>& rows)
{
    const auto* p = reinterpret_cast<const unsigned char*>(d.data());
    if (d.size() < 30) return;
    if (!std::memcmp(p + 12, "VP8 ", 4)) addSize(rows, le16(p + 26) & 0x3fff, le16(p + 28) & 0x3fff);
    else if (!std::memcmp(p + 12, "VP8L", 4)) {
        const std::uint32_t bits = le32(p + 21);
        addSize(rows, (bits & 0x3fff) + 1, ((bits >> 14) & 0x3fff) + 1);
    } else if (!std::memcmp(p + 12, "VP8X", 4)) {
        addSize(rows, le24(p + 24) + 1, le24(p + 27) + 1);
    }
}

// ---- EXIF: a small TIFF file with numbered tags ----
class Exif {
public:
    Exif(const unsigned char* data, std::size_t size) : m_data(data), m_size(size) {}

    void read(std::vector<PropertyRow>& rows)
    {
        if (m_size < 8) return;
        if (!std::memcmp(m_data, "II", 2)) m_little = true;
        else if (!std::memcmp(m_data, "MM", 2)) m_little = false;
        else return;
        readDirectory(u32(4), rows, 0);
    }

private:
    std::uint16_t u16(std::size_t at) const { return m_little ? le16(m_data + at) : be16(m_data + at); }
    std::uint32_t u32(std::size_t at) const { return m_little ? le32(m_data + at) : be32(m_data + at); }

    static std::size_t typeSize(int type)
    {
        switch (type) {
        case 1: case 2: case 7: return 1;
        case 3: return 2;
        case 4: case 9: return 4;
        case 5: case 10: return 8;
        default: return 0;
        }
    }

    std::string valueText(std::size_t entry, int type, std::uint32_t count) const
    {
        const std::size_t bytes = typeSize(type) * count;
        if (bytes == 0) return {};
        const std::size_t at = bytes <= 4 ? entry + 8 : u32(entry + 8);
        if (at + bytes > m_size) return {};
        if (type == 2) return std::string(reinterpret_cast<const char*>(m_data + at), strnlen(reinterpret_cast<const char*>(m_data + at), count));
        if (type == 3) return std::to_string(u16(at));
        if (type == 4) return std::to_string(u32(at));
        if (type == 5 || type == 10) {
            const std::uint32_t top = u32(at), bottom = u32(at + 4);
            if (bottom == 0) return {};
            char text[32];
            if (top % bottom == 0) std::snprintf(text, sizeof text, "%u", top / bottom);
            else if (top < bottom) std::snprintf(text, sizeof text, "%u/%u", top, bottom);
            else std::snprintf(text, sizeof text, "%.2f", double(top) / bottom);
            return text;
        }
        return {};
    }

    void readDirectory(std::size_t at, std::vector<PropertyRow>& rows, int depth)
    {
        if (depth > 2 || at + 2 > m_size) return;
        const int count = u16(at);
        for (int i = 0; i < count; ++i) {
            const std::size_t entry = at + 2 + static_cast<std::size_t>(i) * 12;
            if (entry + 12 > m_size) return;
            const int tag = u16(entry), type = u16(entry + 2);
            const std::uint32_t values = u32(entry + 4);
            if (tag == 0x8769 && type == 4) { readDirectory(u32(entry + 8), rows, depth + 1); continue; }
            if (tag == 0x8825) { rows.push_back({"GPS position", "Stored in this file"}); continue; }

            const char* label = labelOf(tag);
            if (!label) continue;
            std::string value = valueText(entry, type, values);
            if (tag == 0x829a) value += " s";
            if (tag == 0x829d) value = "f/" + value;
            if (tag == 0x920a) value += " mm";
            if (!value.empty()) rows.push_back({label, value});
        }
    }

    static const char* labelOf(int tag)
    {
        switch (tag) {
        case 0x010f: return "Camera maker";
        case 0x0110: return "Camera model";
        case 0x0112: return "Orientation";
        case 0x0131: return "Software";
        case 0x0132: return "Date changed";
        case 0x013b: return "Artist";
        case 0x8298: return "Copyright";
        case 0x829a: return "Exposure time";
        case 0x829d: return "Aperture";
        case 0x8827: return "ISO";
        case 0x9003: return "Date taken";
        case 0x920a: return "Focal length";
        case 0xa434: return "Lens";
        default: return nullptr;
        }
    }

    const unsigned char* m_data;
    std::size_t m_size;
    bool m_little = true;
};

void readJpeg(const std::string& d, std::vector<PropertyRow>& rows)
{
    const auto* p = reinterpret_cast<const unsigned char*>(d.data());
    std::size_t at = 2;
    bool sized = false;
    while (at + 4 <= d.size() && p[at] == 0xFF) {
        const int marker = p[at + 1];
        if (marker == 0xDA || marker == 0xD9) break;
        const std::size_t length = be16(p + at + 2);
        if (length < 2) break;

        const bool frame = marker >= 0xC0 && marker <= 0xCF && marker != 0xC4 && marker != 0xC8 && marker != 0xCC;
        if (frame && !sized && at + 9 < d.size()) {
            addSize(rows, be16(p + at + 7), be16(p + at + 5));
            rows.push_back({"Bit depth", std::to_string(p[at + 4]) + " bits per channel"});
            rows.push_back({"Colour channels", std::to_string(p[at + 9])});
            sized = true;
        }
        if (marker == 0xE1 && length > 8 && at + 2 + length <= d.size() && !std::memcmp(p + at + 4, "Exif\0\0", 6))
            Exif(p + at + 10, length - 8).read(rows);
        at += 2 + length;
    }
}

} // namespace

namespace ContentInfo {

std::vector<PropertyRow> image(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    std::string data(kReadLimit, '\0');
    file.read(data.data(), static_cast<std::streamsize>(data.size()));
    data.resize(static_cast<std::size_t>(file.gcount()));

    std::vector<PropertyRow> rows;
    if (data.rfind("\x89PNG", 0) == 0) readPng(data, rows);
    else if (data.rfind("\xFF\xD8", 0) == 0) readJpeg(data, rows);
    else if (data.rfind("GIF8", 0) == 0) readGif(data, rows);
    else if (data.rfind("BM", 0) == 0) readBmp(data, rows);
    else if (data.rfind("RIFF", 0) == 0 && data.compare(8, 4, "WEBP") == 0) readWebp(data, rows);
    return rows;
}

} // namespace ContentInfo
