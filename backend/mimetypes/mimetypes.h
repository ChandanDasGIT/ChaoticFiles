#pragma once
#include <string>
#include <vector>

// The system's list of file types (shared-mime-info, the freedesktop.org standard). It goes by file name and
// reads no file, so it is cheap enough for a folder with many files. Without the list, everything returns empty.
namespace MimeTypes {

// Reads the list from these folders instead of the system's. For the tests.
void useFolders(const std::vector<std::string>& dataFolders);

// Every type this name could be, best first. More than one means the name alone cannot tell.
std::vector<std::string> candidates(const std::string& fileName);
std::string fromName(const std::string& fileName);    // the type, if the name settles it
bool isExactName(const std::string& fileName);        // the list knows this whole name: Makefile, CMakeLists.txt

// The type and what it is a kind of: text/x-c++src, text/x-csrc, text/plain.
std::vector<std::string> chain(const std::string& mimeType);
// The same, with the other names each one goes by. Apps may list a type under any of them.
std::vector<std::string> related(const std::string& mimeType);

std::string description(const std::string& mimeType);                // "Rust source code", or empty

} // namespace MimeTypes
