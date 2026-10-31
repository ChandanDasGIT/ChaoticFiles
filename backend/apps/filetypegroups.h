#pragma once
#include <string>
#include <vector>

struct FileTypeChoice {
    std::string label;                     // "MP3"
    std::vector<std::string> mimeTypes;    // every name this type goes by; the first is the main one
};

struct FileTypeGroup {
    std::string name;                      // "Music"
    std::vector<FileTypeChoice> types;
};

// The file types the Default Apps panel offers, by category.
const std::vector<FileTypeGroup>& fileTypeGroups();

// A readable name for a type: "Plain Text" for text/plain. Types not in the list keep their own name.
std::string fileTypeLabel(const std::string& mimeType);
