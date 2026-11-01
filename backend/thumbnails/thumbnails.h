#pragma once
#include <functional>
#include <string>

// Thumbnails, in the cache other Linux apps share (the freedesktop.org thumbnail specification).
namespace Thumbnails {

std::string cachePath(const std::string& path);      // where the thumbnail of this file belongs
std::string failPath(const std::string& path);       // where it is noted that this file has none, so it isn't tried again
bool ensureCacheFolder();
bool canMakeAlbumArt();                              // ffmpeg is installed

// Makes a thumbnail with another program, for files Qt can't read: video with ffmpegthumbnailer, PDF with pdftoppm,
// and for audio the picture inside the file with ffmpeg, or else a cover image in its folder. AlbumArt is the quick
// way for audio; this is for the formats it does not read.
// `kind` is "video", "pdf" or "audio". Writes a PNG to `output`.
bool generate(const std::string& path, const std::string& kind, const std::string& output, int size,
              const std::function<bool()>& cancelled);

} // namespace Thumbnails
