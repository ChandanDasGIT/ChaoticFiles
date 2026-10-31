#include "apps/filetypegroups.h"
#include "mimetypes/mimetypes.h"

#include <algorithm>

const std::vector<FileTypeGroup>& fileTypeGroups()
{
    static const std::vector<FileTypeGroup> groups = {
        {"Music", {
            {"MP3", {"audio/mpeg"}},
            {"FLAC", {"audio/flac", "audio/x-flac"}},
            {"WAV", {"audio/wav", "audio/x-wav"}},
            {"OGG", {"audio/ogg", "audio/x-vorbis+ogg"}},
            {"Opus", {"audio/opus", "audio/x-opus+ogg"}},
            {"AAC", {"audio/aac"}},
            {"M4A", {"audio/mp4", "audio/x-m4a"}},
            {"WMA", {"audio/x-ms-wma"}},
        }},
        {"Videos", {
            {"MP4", {"video/mp4"}},
            {"MKV", {"video/x-matroska"}},
            {"WebM", {"video/webm"}},
            {"AVI", {"video/x-msvideo", "video/avi"}},
            {"MOV", {"video/quicktime"}},
            {"WMV", {"video/x-ms-wmv"}},
            {"MPEG", {"video/mpeg"}},
            {"FLV", {"video/x-flv"}},
        }},
        {"Images", {
            {"PNG", {"image/png"}},
            {"JPEG", {"image/jpeg"}},
            {"GIF", {"image/gif"}},
            {"WebP", {"image/webp"}},
            {"SVG", {"image/svg+xml"}},
            {"BMP", {"image/bmp"}},
            {"TIFF", {"image/tiff"}},
            {"HEIC", {"image/heic"}},
            {"AVIF", {"image/avif"}},
        }},
        {"Documents", {
            {"PDF", {"application/pdf"}},
            {"Word (DOCX)", {"application/vnd.openxmlformats-officedocument.wordprocessingml.document"}},
            {"Word (DOC)", {"application/msword"}},
            {"OpenDocument Text", {"application/vnd.oasis.opendocument.text"}},
            {"Excel (XLSX)", {"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"}},
            {"OpenDocument Spreadsheet", {"application/vnd.oasis.opendocument.spreadsheet"}},
            {"PowerPoint (PPTX)", {"application/vnd.openxmlformats-officedocument.presentationml.presentation"}},
            {"OpenDocument Presentation", {"application/vnd.oasis.opendocument.presentation"}},
            {"EPUB", {"application/epub+zip"}},
            {"RTF", {"application/rtf", "text/rtf"}},
        }},
        {"Text & Code", {
            {"Plain Text", {"text/plain"}},
            {"Markdown", {"text/markdown"}},
            {"HTML", {"text/html"}},
            {"CSS", {"text/css"}},
            {"JavaScript", {"text/javascript", "application/javascript"}},
            {"JSON", {"application/json"}},
            {"XML", {"application/xml", "text/xml"}},
            {"Python", {"text/x-python", "text/x-python3"}},
            {"Shell Script", {"application/x-shellscript"}},
            {"C and C++", {"text/x-csrc", "text/x-c++src", "text/x-chdr", "text/x-c++hdr"}},
            {"QML", {"text/x-qml"}},
            {"TypeScript", {"text/x-typescript", "text/x-typescript-jsx"}},
            {"Rust", {"text/rust"}},
            {"Go", {"text/x-go"}},
            {"Java", {"text/x-java"}},
            {"CMake", {"text/x-cmake"}},
            {"YAML", {"application/yaml"}},
            {"TOML", {"application/toml"}},
        }},
        {"Archives", {
            {"ZIP", {"application/zip"}},
            {"TAR", {"application/x-tar"}},
            {"GZip", {"application/gzip"}},
            {"XZ", {"application/x-xz"}},
            {"7-Zip", {"application/x-7z-compressed"}},
            {"RAR", {"application/vnd.rar", "application/x-rar"}},
            {"Zstandard", {"application/zstd"}},
        }},
    };
    return groups;
}

std::string fileTypeLabel(const std::string& mimeType)
{
    for (const FileTypeGroup& group : fileTypeGroups())
        for (const FileTypeChoice& type : group.types)
            if (std::find(type.mimeTypes.begin(), type.mimeTypes.end(), mimeType) != type.mimeTypes.end())
                return type.label;

    std::string name = MimeTypes::description(mimeType);              // "Qt Markup Language file" reads badly before "files"
    const std::string tail = " file";
    if (name.size() > tail.size() && name.compare(name.size() - tail.size(), tail.size(), tail) == 0) name.resize(name.size() - tail.size());
    return name.empty() ? mimeType : name;
}
