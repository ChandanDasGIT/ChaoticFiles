#include "contentinfo/contentinfo.h"

#include "format/format.h"
#include "process/process.h"

#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>

namespace {

std::string clockTime(double seconds)
{
    const auto total = static_cast<long long>(seconds + 0.5);
    char text[32];
    if (total >= 3600) std::snprintf(text, sizeof text, "%lld:%02lld:%02lld", total / 3600, total / 60 % 60, total % 60);
    else std::snprintf(text, sizeof text, "%lld:%02lld", total / 60, total % 60);
    return text;
}

std::string kilobits(const std::string& bitsPerSecond)
{
    return Format::number(std::strtoull(bitsPerSecond.c_str(), nullptr, 10) / 1000) + " kbit/s";
}

std::string frameRate(const std::string& ratio)
{
    const auto slash = ratio.find('/');
    if (slash == std::string::npos) return ratio;
    const double top = std::atof(ratio.c_str()), bottom = std::atof(ratio.c_str() + slash + 1);
    if (bottom <= 0) return {};
    char text[32];
    std::snprintf(text, sizeof text, "%.3g fps", top / bottom);
    return text;
}

std::string capitalized(std::string text)
{
    if (!text.empty()) text[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[0])));
    return text;
}

} // namespace

namespace ContentInfo {

std::vector<PropertyRow> media(const std::string& path, const Cancelled& cancelled)
{
    if (Process::findProgram("ffprobe").empty()) return {{"Media details", "Not available: ffprobe is not installed"}};

    const auto result = Process::run({"ffprobe", "-v", "error", "-show_format", "-show_streams", "-of", "flat", "--", path}, cancelled);
    if (result.exitCode != 0) return {{"Media details", "ffprobe could not read this file"}};

    // lines look like streams.stream.0.codec_name="h264" and format.tags.title="Song"
    std::map<std::string, std::string> format, tags;
    std::map<int, std::map<std::string, std::string>> streams;
    std::istringstream lines(result.output);
    std::string line;
    while (std::getline(lines, line)) {
        const auto equals = line.find('=');
        if (equals == std::string::npos) continue;
        std::string key = line.substr(0, equals), value = line.substr(equals + 1);
        if (value.size() >= 2 && value.front() == '"') value = value.substr(1, value.size() - 2);

        if (key.rfind("format.tags.", 0) == 0) tags[key.substr(12)] = value;
        else if (key.rfind("format.", 0) == 0) format[key.substr(7)] = value;
        else if (key.rfind("streams.stream.", 0) == 0) {
            const std::string rest = key.substr(15);
            const auto dot = rest.find('.');
            if (dot != std::string::npos) streams[std::atoi(rest.c_str())][rest.substr(dot + 1)] = value;
        }
    }

    std::vector<PropertyRow> rows;
    if (format.count("format_long_name")) rows.push_back({"Container", format["format_long_name"]});
    if (format.count("duration")) rows.push_back({"Duration", clockTime(std::atof(format["duration"].c_str()))});
    if (format.count("bit_rate")) rows.push_back({"Bit rate", kilobits(format["bit_rate"])});

    for (auto& [index, stream] : streams) {
        const std::string prefix = capitalized(stream["codec_type"]) + " " + std::to_string(index) + ": ";
        std::string codec = stream.count("codec_long_name") ? stream["codec_long_name"] : stream["codec_name"];
        rows.push_back({prefix + "codec", codec});
        if (stream["codec_type"] == "video") {
            rows.push_back({prefix + "size", stream["width"] + " × " + stream["height"] + " pixels"});
            const std::string rate = frameRate(stream["avg_frame_rate"]);
            if (!rate.empty()) rows.push_back({prefix + "frame rate", rate});
            if (stream.count("pix_fmt")) rows.push_back({prefix + "pixel format", stream["pix_fmt"]});
        } else if (stream["codec_type"] == "audio") {
            if (stream.count("sample_rate")) rows.push_back({prefix + "sample rate", Format::number(std::strtoull(stream["sample_rate"].c_str(), nullptr, 10)) + " Hz"});
            if (stream.count("channels")) rows.push_back({prefix + "channels", stream["channels"] + (stream.count("channel_layout") ? " (" + stream["channel_layout"] + ")" : "")});
            if (stream.count("bits_per_raw_sample") && stream["bits_per_raw_sample"] != "N/A") rows.push_back({prefix + "bit depth", stream["bits_per_raw_sample"] + " bits"});
        }
        if (stream.count("bit_rate") && stream["bit_rate"] != "N/A") rows.push_back({prefix + "bit rate", kilobits(stream["bit_rate"])});
    }
    for (const auto& [name, value] : tags) rows.push_back({"Tag: " + capitalized(name), value});
    return rows;
}

} // namespace ContentInfo
