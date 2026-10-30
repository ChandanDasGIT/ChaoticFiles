#pragma once
#include <string>

// Detailed file types from libmagic, the library behind the `file` command. It is loaded when first needed,
// so the app works without it. Everything here returns empty text if the library is missing.
namespace MagicFile {

bool available();
std::string describe(const std::string& path);      // "ELF 64-bit LSB pie executable, x86-64, ..."
std::string mimeType(const std::string& path);      // "image/png"
std::string encoding(const std::string& path);      // "utf-8", "binary"

} // namespace MagicFile
