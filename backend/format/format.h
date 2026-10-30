#pragma once
#include <cstdint>
#include <string>

// Text for the screen. Sizes use 1024 steps and say so: 1.5 MiB.
namespace Format {

std::string size(std::uint64_t bytes);
std::string date(std::int64_t secondsSince1970);     // local time, 2026-10-30 23:32
std::string itemCount(std::size_t count);            // "1 item", "12,345 items"

} // namespace Format
