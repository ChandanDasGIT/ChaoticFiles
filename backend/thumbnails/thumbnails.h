#pragma once
#include <functional>
#include <string>

// Thumbnails, in the cache other Linux apps share (the freedesktop.org thumbnail specification).
namespace Thumbnails {

std::string cachePath(const std::string& path);      // where the thumbnail of this file belongs
bool ensureCacheFolder();

// Makes a thumbnail with another program, for files Qt can't read: video with ffmpegthumbnailer, PDF with pdftoppm.
// `kind` is "video" or "pdf". Writes a PNG to `output`.
bool generate(const std::string& path, const std::string& kind, const std::string& output, int size,
              const std::function<bool()>& cancelled);

} // namespace Thumbnails
