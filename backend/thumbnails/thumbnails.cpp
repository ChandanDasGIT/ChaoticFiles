#include "thumbnails/thumbnails.h"

#include "albumart/albumart.h"
#include "hashing/hashing.h"
#include "pathbar/pathbar.h"
#include "process/process.h"

#include <cstdlib>
#include <filesystem>
#include <sys/stat.h>
#include <system_error>

namespace {

std::string cacheRoot()
{
    const char* cache = std::getenv("XDG_CACHE_HOME");
    return (cache && *cache ? std::string(cache) : PathBar::homeDirectory() + "/.cache") + "/thumbnails";
}

bool writeScaledPicture(const std::string& source, const std::string& output, int size, const std::function<bool()>& cancelled)
{
    const std::string pixels = std::to_string(size);
    return Process::run({"ffmpeg", "-y", "-loglevel", "error", "-i", source, "-map", "0:v:0", "-frames:v", "1",
                         "-vf", "scale=" + pixels + ":" + pixels + ":force_original_aspect_ratio=decrease",
                         "-f", "image2", "-c:v", "png", output}, cancelled).exitCode == 0;
}

} // namespace

namespace Thumbnails {

std::string cachePath(const std::string& path)
{
    return cacheRoot() + "/large/" + Hashing::hashText("MD5", PathBar::fileUrl(path)) + ".png";
}

std::string failPath(const std::string& path)
{
    return cacheRoot() + "/fail/chaoticfiles/" + Hashing::hashText("MD5", PathBar::fileUrl(path)) + ".png";
}

bool ensureCacheFolder()
{
    std::error_code error;
    std::filesystem::create_directories(cacheRoot() + "/large", error);
    std::filesystem::create_directories(cacheRoot() + "/fail/chaoticfiles", error);
    if (error) return false;
    for (const char* folder : {"", "/large", "/fail", "/fail/chaoticfiles"}) chmod((cacheRoot() + folder).c_str(), 0700);
    return true;
}

bool canMakeAlbumArt()
{
    return !Process::findProgram("ffmpeg").empty();
}

bool generate(const std::string& path, const std::string& kind, const std::string& output, int size,
              const std::function<bool()>& cancelled)
{
    const std::string pixels = std::to_string(size);
    if (kind == "video") {
        const auto result = Process::run({"ffmpegthumbnailer", "-i", path, "-o", output, "-s", pixels, "-c", "png"}, cancelled);
        if (result.exitCode == 0) return true;
        const auto fallback = Process::run({"ffmpeg", "-y", "-loglevel", "error", "-ss", "3", "-i", path, "-frames:v", "1",
                                            "-vf", "scale=" + pixels + ":-1", "-f", "image2", "-c:v", "png", output}, cancelled);
        return fallback.exitCode == 0;
    }
    if (kind == "audio") {
        if (writeScaledPicture(path, output, size, cancelled)) return true;
        const std::string cover = AlbumArt::coverBeside(path);
        return !cover.empty() && writeScaledPicture(cover, output, size, cancelled);
    }
    if (kind == "pdf") {
        // pdftoppm adds .png to the name itself
        std::string stem = output;
        if (stem.size() > 4 && stem.compare(stem.size() - 4, 4, ".png") == 0) stem.resize(stem.size() - 4);
        return Process::run({"pdftoppm", "-singlefile", "-png", "-scale-to", pixels, path, stem}, cancelled).exitCode == 0;
    }
    return false;
}

} // namespace Thumbnails
