#pragma once
#include <cstdint>
#include <functional>
#include <string>

struct Outcome {
    bool ok = true;
    std::string error;             // a sentence for the user when ok is false
};

// The small changes to the disk. None of these replaces anything that already exists.
namespace FileOps {

Outcome createFolder(const std::string& path);
Outcome createFile(const std::string& path);
Outcome createLink(const std::string& target, const std::string& linkPath);
Outcome rename(const std::string& from, const std::string& to);       // both in the same folder or not

// A name that is free next to `path`: "report (2).txt" if "report.txt" is taken.
std::string uniquePath(const std::string& path);

// A sentence if this name can't be a file name, otherwise empty.
std::string nameProblem(const std::string& name);

// Folders that must never be deleted or trashed: /, the folders directly under it, Home and everything above it.
bool isProtected(const std::string& path);

// Deletes for good. Links are removed, never followed, and it refuses to cross into another mounted filesystem.
Outcome removeTree(const std::string& path, const std::function<bool()>& cancelled);

bool onSameDevice(const std::string& a, const std::string& b);

} // namespace FileOps
