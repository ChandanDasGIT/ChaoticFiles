#pragma once
#include "entry/entry.h"

// Fills in entry.category and entry.typeName from the kind, name and permissions.
// It does not read the file, so it is cheap enough for a folder with many files.
void detectFileType(Entry& entry);

// The category as text, for the front end: "folder", "image", ...
const char* categoryName(FileCategory category);
