#pragma once
#include <string>

// The picture of a song, read straight from the start of the file, which is far quicker than starting ffmpeg.
namespace AlbumArt {

struct Found {
    bool understood = false;     // false: a format not read here, or a tag too odd to trust, so ffmpeg should try
    std::string picture;         // the image file as it is stored, usually a JPEG. Empty if the song has none.
};

Found read(const std::string& path);             // FLAC, and anything with an ID3v2 tag such as MP3
std::string coverBeside(const std::string& path);    // cover.jpg, Folder.PNG and the like in the song's folder, or empty

} // namespace AlbumArt
