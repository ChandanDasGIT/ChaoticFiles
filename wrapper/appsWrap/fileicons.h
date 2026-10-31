#pragma once
#include <QString>

struct Entry;

namespace FileIcons {

// The icon theme's icon for this file's type, or empty. Empty means the app draws its own from Icons.qml:
// folders always, and any type the theme has no icon for.
QString themeIconFor(const Entry& entry);

} // namespace FileIcons
