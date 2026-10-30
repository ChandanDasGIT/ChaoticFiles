#include "contentinfo/contentinfo.h"

#include "format/format.h"

#include <fstream>

namespace {
constexpr std::size_t kLimit = 128 * 1024 * 1024;
}

namespace ContentInfo {

std::vector<PropertyRow> text(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file) return {};

    std::uint64_t lines = 0, words = 0, characters = 0, crlf = 0, lf = 0, cr = 0, longest = 0, current = 0, bytes = 0;
    bool inWord = false, truncated = false;
    char previous = 0, first[3] = {0, 0, 0};
    std::vector<char> buffer(1024 * 1024);

    while (file) {
        file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const auto got = static_cast<std::size_t>(file.gcount());
        if (bytes == 0)
            for (std::size_t i = 0; i < 3 && i < got; ++i) first[i] = buffer[i];
        for (std::size_t i = 0; i < got; ++i) {
            const char c = buffer[i];
            if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
                ++characters;
                if (c != '\n' && c != '\r') ++current;
            }
            if (c == '\n') {
                ++lines;
                longest = std::max(longest, current);
                current = 0;
                if (previous == '\r') { ++crlf; --cr; }
                else ++lf;
            } else if (c == '\r') {
                ++cr;
            }
            const bool space = c == ' ' || c == '\n' || c == '\t' || c == '\r';
            if (!space && !inWord) ++words;
            inWord = !space;
            previous = c;
        }
        bytes += got;
        if (bytes >= kLimit) {
            truncated = true;
            break;
        }
    }
    if (current > 0) {
        ++lines;
        longest = std::max(longest, current);
    }

    std::string endings;
    const auto add = [&endings](std::uint64_t count, const char* name) {
        if (count) endings += (endings.empty() ? "" : ", ") + std::string(name) + " (" + Format::number(count) + ")";
    };
    add(lf, "LF");
    add(crlf, "CRLF");
    add(cr, "CR");

    std::vector<PropertyRow> rows;
    const std::string note = truncated ? " (first 128 MiB)" : "";
    rows.push_back({"Lines", Format::number(lines) + note});
    rows.push_back({"Words", Format::number(words) + note});
    rows.push_back({"Characters", Format::number(characters) + note});
    rows.push_back({"Longest line", Format::number(longest) + " characters"});
    rows.push_back({"Line endings", endings.empty() ? "None" : endings});
    const bool bom = first[0] == '\xEF' && first[1] == '\xBB' && first[2] == '\xBF';
    rows.push_back({"Byte order mark", bom ? "Yes" : "No"});
    return rows;
}

} // namespace ContentInfo
