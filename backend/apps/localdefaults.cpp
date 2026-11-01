#include "apps/localdefaults.h"

#include "mimetypes/mimetypes.h"
#include "pathbar/pathbar.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

constexpr const char* kSection = "[Default Applications]";

std::string configFolder()
{
    const char* config = std::getenv("XDG_CONFIG_HOME");
    return config && *config ? config : PathBar::homeDirectory() + "/.config";
}

std::vector<std::string> readLines(const std::string& path)
{
    std::vector<std::string> lines;
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }
    return lines;
}

bool isSectionStart(const std::string& line) { return !line.empty() && line.front() == '['; }

std::string modeMarker() { return configFolder() + "/ChaoticFiles/use-local-defaults"; }

} // namespace

namespace LocalDefaults {

std::string filePath()
{
    return configFolder() + "/ChaoticFiles/mimeapps.list";
}

bool inUse()
{
    std::error_code error;
    return std::filesystem::exists(modeMarker(), error);
}

void setInUse(bool local)
{
    const std::string path = modeMarker();
    std::error_code error;
    if (!local) {
        std::filesystem::remove(path, error);
        return;
    }
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), error);
    std::ofstream marker(path);
}

bool seed()
{
    const std::string path = filePath();
    std::error_code error;
    if (std::filesystem::exists(path, error)) return false;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), error);

    std::ifstream system(configFolder() + "/mimeapps.list", std::ios::binary);
    std::ofstream copy(path, std::ios::binary);
    if (system) copy << system.rdbuf();
    else copy << kSection << '\n';
    return copy.good();
}

std::vector<std::string> appsFor(const std::string& mime)
{
    const std::vector<std::string> names = MimeTypes::names(mime);
    std::vector<std::string> apps;
    bool inSection = false;
    for (const std::string& line : readLines(filePath())) {
        if (isSectionStart(line)) inSection = line == kSection;
        else if (inSection) {
            const auto equals = line.find('=');
            if (equals == std::string::npos || std::find(names.begin(), names.end(), line.substr(0, equals)) == names.end()) continue;
            std::istringstream ids(line.substr(equals + 1));
            std::string id;
            while (std::getline(ids, id, ';'))
                if (!id.empty()) apps.push_back(id);
        }
    }
    return apps;
}

bool set(const std::string& appId, const std::vector<std::string>& mimes)
{
    if (mimes.empty()) return false;
    const std::string path = filePath();
    const auto isChosenType = [&mimes](const std::string& line) {
        return std::any_of(mimes.begin(), mimes.end(), [&line](const std::string& mime) { return line.rfind(mime + "=", 0) == 0; });
    };

    // every line stays as it was, except the entries of these types, which are written again at the top of the section
    std::vector<std::string> lines = readLines(path), kept;
    bool inSection = false, placed = false;
    const auto place = [&] {
        for (const std::string& mime : mimes) kept.push_back(mime + "=" + appId);
        placed = true;
    };
    for (const std::string& line : lines) {
        if (isSectionStart(line)) inSection = line == kSection;
        if (inSection && isChosenType(line)) continue;
        kept.push_back(line);
        if (inSection && line == kSection) place();
    }
    if (!placed) {
        kept.push_back(kSection);
        place();
    }

    // written beside the list and moved over it, so a failure can't leave half a file
    const std::string temporary = path + ".part";
    {
        std::ofstream file(temporary, std::ios::trunc);
        for (const std::string& line : kept) file << line << '\n';
        if (!file.good()) return false;
    }
    return std::rename(temporary.c_str(), path.c_str()) == 0;
}

} // namespace LocalDefaults
