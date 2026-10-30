#include "contentinfo/contentinfo.h"

#include "magic/magicfile.h"

#include <algorithm>

namespace {

bool startsWith(const std::string& text, const char* prefix) { return text.rfind(prefix, 0) == 0; }

bool isArchive(const std::string& mime)
{
    static const char* const types[] = {
        "application/zip", "application/x-tar", "application/gzip", "application/x-gzip", "application/x-xz",
        "application/x-bzip2", "application/zstd", "application/x-7z-compressed", "application/x-rar",
        "application/vnd.rar", "application/java-archive",
    };
    return std::find(std::begin(types), std::end(types), mime) != std::end(types);
}

bool isProgram(const std::string& mime)
{
    return mime == "application/x-executable" || mime == "application/x-pie-executable"
        || mime == "application/x-sharedlib" || mime == "application/x-object" || mime == "application/x-coredump";
}

void append(std::vector<PropertyRow>& rows, std::vector<PropertyRow> more)
{
    rows.insert(rows.end(), std::make_move_iterator(more.begin()), std::make_move_iterator(more.end()));
}

} // namespace

namespace ContentInfo {

std::vector<PropertyRow> collect(const std::string& path, const Cancelled& cancelled)
{
    std::vector<PropertyRow> rows;
    if (!MagicFile::available()) {
        rows.push_back({"Detailed type", "Not available: libmagic is not installed"});
        return rows;
    }
    const std::string mime = MagicFile::mimeType(path);
    rows.push_back({"MIME type", mime});
    rows.push_back({"Detailed type", MagicFile::describe(path)});
    rows.push_back({"Encoding", MagicFile::encoding(path)});

    if (startsWith(mime, "text/") || mime == "application/json" || mime == "application/xml") append(rows, text(path));
    else if (startsWith(mime, "image/")) append(rows, image(path));
    else if (startsWith(mime, "audio/") || startsWith(mime, "video/")) append(rows, media(path, cancelled));
    else if (mime == "application/pdf") append(rows, pdf(path, cancelled));
    else if (isProgram(mime)) append(rows, elf(path));
    else if (isArchive(mime)) append(rows, archive(path, mime, cancelled));
    return rows;
}

} // namespace ContentInfo
