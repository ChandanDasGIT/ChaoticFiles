#include "filetype/filetype.h"
#include "mimetypes/mimetypes.h"

#include <algorithm>
#include <cctype>
#include <string_view>
#include <unordered_map>

namespace {

struct KnownType {
    FileCategory category;
    const char* name;
};

const std::unordered_map<std::string_view, KnownType>& knownTypes()
{
    using C = FileCategory;
    static const std::unordered_map<std::string_view, KnownType> types = {
        {"txt", {C::Text, "Text File"}}, {"md", {C::Text, "Markdown Document"}}, {"log", {C::Text, "Log File"}},
        {"rst", {C::Text, "reStructuredText File"}}, {"csv", {C::Spreadsheet, "CSV File"}},
        {"tsv", {C::Spreadsheet, "TSV File"}},

        {"c", {C::Code, "C Source"}}, {"h", {C::Code, "C Header"}}, {"cpp", {C::Code, "C++ Source"}},
        {"cc", {C::Code, "C++ Source"}}, {"cxx", {C::Code, "C++ Source"}}, {"hpp", {C::Code, "C++ Header"}},
        {"rs", {C::Code, "Rust Source"}}, {"go", {C::Code, "Go Source"}}, {"py", {C::Code, "Python Script"}},
        {"js", {C::Code, "JavaScript File"}}, {"mjs", {C::Code, "JavaScript Module"}},
        {"ts", {C::Code, "TypeScript File"}}, {"jsx", {C::Code, "JSX File"}}, {"tsx", {C::Code, "TSX File"}},
        {"java", {C::Code, "Java Source"}}, {"kt", {C::Code, "Kotlin Source"}}, {"lua", {C::Code, "Lua Script"}},
        {"rb", {C::Code, "Ruby Script"}}, {"php", {C::Code, "PHP Script"}}, {"cs", {C::Code, "C# Source"}},
        {"swift", {C::Code, "Swift Source"}}, {"qml", {C::Code, "QML File"}}, {"sh", {C::Code, "Shell Script"}},
        {"bash", {C::Code, "Bash Script"}}, {"zsh", {C::Code, "Zsh Script"}}, {"fish", {C::Code, "Fish Script"}},
        {"cmake", {C::Code, "CMake File"}}, {"html", {C::Code, "HTML Document"}}, {"htm", {C::Code, "HTML Document"}},
        {"css", {C::Code, "CSS Stylesheet"}}, {"json", {C::Code, "JSON File"}}, {"xml", {C::Code, "XML File"}},
        {"yaml", {C::Code, "YAML File"}}, {"yml", {C::Code, "YAML File"}}, {"toml", {C::Code, "TOML File"}},
        {"ini", {C::Code, "Configuration File"}}, {"conf", {C::Code, "Configuration File"}},
        {"sql", {C::Code, "SQL Script"}}, {"hh", {C::Code, "C++ Header"}}, {"hxx", {C::Code, "C++ Header"}},
        {"vue", {C::Code, "Vue Component"}}, {"svelte", {C::Code, "Svelte Component"}}, {"scss", {C::Code, "Sass Stylesheet"}},
        {"sass", {C::Code, "Sass Stylesheet"}}, {"less", {C::Code, "Less Stylesheet"}}, {"dart", {C::Code, "Dart Source"}},
        {"zig", {C::Code, "Zig Source"}}, {"nim", {C::Code, "Nim Source"}}, {"pl", {C::Code, "Perl Script"}},
        {"hs", {C::Code, "Haskell Source"}}, {"ex", {C::Code, "Elixir Source"}}, {"exs", {C::Code, "Elixir Script"}},
        {"scala", {C::Code, "Scala Source"}}, {"r", {C::Code, "R Script"}}, {"jl", {C::Code, "Julia Source"}},
        {"asm", {C::Code, "Assembly Source"}}, {"s", {C::Code, "Assembly Source"}}, {"vala", {C::Code, "Vala Source"}},
        {"ps1", {C::Code, "PowerShell Script"}}, {"bat", {C::Code, "Batch File"}}, {"proto", {C::Code, "Protocol Buffers File"}},
        {"tf", {C::Code, "Terraform File"}}, {"gradle", {C::Code, "Gradle Script"}}, {"qrc", {C::Code, "Qt Resource File"}},
        {"ui", {C::Code, "Qt Designer File"}}, {"pro", {C::Code, "Qt Project File"}}, {"tex", {C::Text, "TeX Document"}},
        {"jsonc", {C::Code, "JSON File"}}, {"ipynb", {C::Code, "Jupyter Notebook"}},

        {"png", {C::Image, "PNG Image"}}, {"jpg", {C::Image, "JPEG Image"}}, {"jpeg", {C::Image, "JPEG Image"}},
        {"gif", {C::Image, "GIF Image"}}, {"webp", {C::Image, "WebP Image"}}, {"svg", {C::Image, "SVG Image"}},
        {"bmp", {C::Image, "Bitmap Image"}}, {"tiff", {C::Image, "TIFF Image"}}, {"tif", {C::Image, "TIFF Image"}},
        {"ico", {C::Image, "Icon"}}, {"heic", {C::Image, "HEIC Image"}}, {"avif", {C::Image, "AVIF Image"}},
        {"psd", {C::Image, "Photoshop Image"}}, {"xcf", {C::Image, "GIMP Image"}},

        {"mp3", {C::Audio, "MP3 Audio"}}, {"flac", {C::Audio, "FLAC Audio"}}, {"wav", {C::Audio, "WAV Audio"}},
        {"ogg", {C::Audio, "Ogg Audio"}}, {"opus", {C::Audio, "Opus Audio"}}, {"m4a", {C::Audio, "M4A Audio"}},
        {"aac", {C::Audio, "AAC Audio"}}, {"wma", {C::Audio, "WMA Audio"}},

        {"mp4", {C::Video, "MP4 Video"}}, {"mkv", {C::Video, "Matroska Video"}}, {"webm", {C::Video, "WebM Video"}},
        {"avi", {C::Video, "AVI Video"}}, {"mov", {C::Video, "QuickTime Video"}}, {"wmv", {C::Video, "WMV Video"}},
        {"m4v", {C::Video, "M4V Video"}}, {"flv", {C::Video, "FLV Video"}},

        {"zip", {C::Archive, "ZIP Archive"}}, {"tar", {C::Archive, "Tar Archive"}}, {"gz", {C::Archive, "Gzip Archive"}},
        {"tgz", {C::Archive, "Gzip Tar Archive"}}, {"xz", {C::Archive, "XZ Archive"}}, {"bz2", {C::Archive, "Bzip2 Archive"}},
        {"zst", {C::Archive, "Zstandard Archive"}}, {"7z", {C::Archive, "7-Zip Archive"}}, {"rar", {C::Archive, "RAR Archive"}},

        {"pdf", {C::Pdf, "PDF Document"}}, {"epub", {C::Document, "EPUB Book"}},
        {"doc", {C::Document, "Word Document"}}, {"docx", {C::Document, "Word Document"}},
        {"odt", {C::Document, "OpenDocument Text"}}, {"rtf", {C::Document, "Rich Text Document"}},
        {"xls", {C::Spreadsheet, "Excel Spreadsheet"}}, {"xlsx", {C::Spreadsheet, "Excel Spreadsheet"}},
        {"ods", {C::Spreadsheet, "OpenDocument Spreadsheet"}},
        {"ppt", {C::Presentation, "PowerPoint Presentation"}}, {"pptx", {C::Presentation, "PowerPoint Presentation"}},
        {"odp", {C::Presentation, "OpenDocument Presentation"}},

        {"appimage", {C::Program, "AppImage Program"}}, {"exe", {C::Program, "Windows Program"}},
        {"msi", {C::Program, "Windows Installer"}}, {"deb", {C::Archive, "Debian Package"}},
        {"rpm", {C::Archive, "RPM Package"}}, {"desktop", {C::Program, "Desktop Entry"}},
        {"so", {C::Program, "Shared Library"}},

        {"iso", {C::DiskImage, "Disc Image"}}, {"img", {C::DiskImage, "Disk Image"}},
        {"qcow2", {C::DiskImage, "QEMU Disk Image"}},

        {"ttf", {C::Font, "TrueType Font"}}, {"otf", {C::Font, "OpenType Font"}},
        {"woff", {C::Font, "Web Font"}}, {"woff2", {C::Font, "Web Font"}},
    };
    return types;
}

std::string extensionOf(const std::string& name)
{
    const auto dot = name.rfind('.');
    if (dot == std::string::npos || dot == 0 || dot + 1 == name.size()) return {};
    std::string ext = name.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    return ext;
}

// what the system's list says, for what the table above doesn't name
FileCategory categoryOf(const std::string& mimeType)
{
    const auto startsWith = [&mimeType](const char* start) { return mimeType.rfind(start, 0) == 0; };
    if (startsWith("image/")) return FileCategory::Image;
    if (startsWith("audio/")) return FileCategory::Audio;
    if (startsWith("video/")) return FileCategory::Video;
    if (startsWith("font/")) return FileCategory::Font;
    if (mimeType == "text/plain") return FileCategory::Text;
    const std::vector<std::string> kinds = MimeTypes::chain(mimeType);
    const bool text = std::find(kinds.begin(), kinds.end(), "text/plain") != kinds.end();
    return text ? FileCategory::Code : FileCategory::Unknown;
}

void useMimeType(Entry& entry, const std::string& mimeType)
{
    entry.category = categoryOf(mimeType);
    std::string name = MimeTypes::description(mimeType);
    if (!name.empty()) name[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
    entry.typeName = name.empty() ? "File" : name;
}

const char* specialKindName(EntryKind kind)
{
    switch (kind) {
    case EntryKind::Socket: return "Socket";
    case EntryKind::Pipe: return "Named Pipe";
    case EntryKind::CharDevice: return "Character Device";
    case EntryKind::BlockDevice: return "Block Device";
    default: return nullptr;
    }
}

} // namespace

void detectFileType(Entry& entry)
{
    if (entry.brokenLink) {
        entry.category = FileCategory::Unknown;
        entry.typeName = "Broken Link";
        return;
    }
    if (entry.isDirectory()) {
        entry.category = FileCategory::Folder;
        entry.typeName = entry.symlink ? "Link to Folder" : "Folder";
        return;
    }
    if (const char* special = specialKindName(entry.kind)) {
        entry.category = FileCategory::System;
        entry.typeName = special;
        return;
    }

    const std::string ext = extensionOf(entry.name);
    const std::string mimeType = MimeTypes::fromName(entry.name);
    if (entry.name == "Dockerfile" || entry.name == "Containerfile") {       // the system's list has neither
        entry.category = FileCategory::Code;
        entry.typeName = "Dockerfile";
    } else if (!mimeType.empty() && MimeTypes::isExactName(entry.name)) {    // Makefile, or CMakeLists.txt before its .txt
        useMimeType(entry, mimeType);
    } else if (const auto it = knownTypes().find(ext); it != knownTypes().end()) {
        entry.category = it->second.category;
        entry.typeName = it->second.name;
    } else if (entry.mode & 0111) {
        entry.category = FileCategory::Program;
        entry.typeName = "Program";
    } else if (!mimeType.empty()) {
        useMimeType(entry, mimeType);
    } else if (ext.empty()) {
        entry.category = FileCategory::Unknown;
        entry.typeName = "File";
    } else {
        std::string upper = ext;
        std::transform(upper.begin(), upper.end(), upper.begin(), [](unsigned char c) { return std::toupper(c); });
        entry.category = FileCategory::Unknown;
        entry.typeName = upper + " File";
    }
    if (entry.symlink) entry.typeName = "Link to " + entry.typeName;
}

std::string fileTag(const Entry& entry)
{
    constexpr std::size_t kLongest = 4;       // what fits on the small tag in a list row
    if (entry.kind != EntryKind::File || entry.brokenLink) return {};
    if (entry.category != FileCategory::Code && entry.category != FileCategory::Text) return {};

    if (entry.name == "Dockerfile" || entry.name == "Containerfile") return "DOCK";
    if (entry.name == "Makefile") return "MAKE";
    if (entry.name == "CMakeLists.txt") return "CMAK";
    std::string tag = extensionOf(entry.name).substr(0, kLongest);
    std::transform(tag.begin(), tag.end(), tag.begin(), [](unsigned char c) { return std::toupper(c); });
    return tag;
}

const char* categoryName(FileCategory category)
{
    switch (category) {
    case FileCategory::Folder: return "folder";
    case FileCategory::Text: return "text";
    case FileCategory::Code: return "code";
    case FileCategory::Image: return "image";
    case FileCategory::Audio: return "audio";
    case FileCategory::Video: return "video";
    case FileCategory::Archive: return "archive";
    case FileCategory::Pdf: return "pdf";
    case FileCategory::Document: return "document";
    case FileCategory::Spreadsheet: return "spreadsheet";
    case FileCategory::Presentation: return "presentation";
    case FileCategory::Program: return "program";
    case FileCategory::DiskImage: return "diskimage";
    case FileCategory::Font: return "font";
    case FileCategory::System: return "system";
    case FileCategory::Unknown: break;
    }
    return "unknown";
}
