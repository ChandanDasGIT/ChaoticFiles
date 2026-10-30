#include "format/format.h"

#include <cstdio>
#include <ctime>

namespace Format {

std::string size(std::uint64_t bytes)
{
    if (bytes < 1024) return std::to_string(bytes) + (bytes == 1 ? " byte" : " bytes");

    static const char* const units[] = {"KiB", "MiB", "GiB", "TiB", "PiB", "EiB"};
    double value = static_cast<double>(bytes) / 1024.0;
    int unit = 0;
    while (value >= 1024.0 && unit < 5) {
        value /= 1024.0;
        ++unit;
    }
    char text[32];
    std::snprintf(text, sizeof text, value < 100.0 ? "%.1f %s" : "%.0f %s", value, units[unit]);
    return text;
}

std::string date(std::int64_t secondsSince1970)
{
    const std::time_t when = static_cast<std::time_t>(secondsSince1970);
    std::tm local {};
    if (!localtime_r(&when, &local)) return {};
    char text[32];
    std::strftime(text, sizeof text, "%Y-%m-%d %H:%M", &local);
    return text;
}

std::string itemCount(std::size_t count)
{
    std::string digits = std::to_string(count);
    for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3)
        digits.insert(static_cast<std::size_t>(i), ",");
    return digits + (count == 1 ? " item" : " items");
}

} // namespace Format
