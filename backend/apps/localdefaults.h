#pragma once
#include <string>
#include <vector>

// This app's own list of default apps, for when a choice should not reach the rest of the system. It has the same
// format as the system's mimeapps.list, and starts as a copy of it. The system's file is only ever read here.
namespace LocalDefaults {

std::string filePath();                  // ~/.config/ChaoticFiles/mimeapps.list
bool seed();                             // makes the copy, the first time only
// Kept in a file: each window is its own process, and a window opened earlier must follow the change too.
bool inUse();
void setInUse(bool local);
// In order of preference, under any name the type goes by. The lines nearest the top are the newest.
std::vector<std::string> appsFor(const std::string& mime);
bool set(const std::string& appId, const std::vector<std::string>& mimes);

} // namespace LocalDefaults
