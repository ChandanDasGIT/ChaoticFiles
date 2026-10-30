#pragma once
#include "fileops/fileops.h"

#include <functional>
#include <string>
#include <vector>

// Compressing and extracting, done by the tar, zip, unzip and 7z programs.
namespace Archiver {

bool isArchive(const std::string& name);                    // by its name
std::string withoutExtension(const std::string& name);      // "photos.tar.gz" gives "photos"

// The kind of archive comes from the end of `archivePath`: .tar.gz .tar.xz .tar.zst .tar .zip .7z
// All `paths` must be in one folder.
Outcome compress(const std::vector<std::string>& paths, const std::string& archivePath,
                 const std::function<bool()>& cancelled);

// Extracts into `folder`, which must already exist.
Outcome extract(const std::string& archivePath, const std::string& folder, const std::function<bool()>& cancelled);

} // namespace Archiver
