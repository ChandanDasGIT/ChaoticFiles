#pragma once
#include "folderscan/folderscan.h"
#include "properties/propertyreport.h"

#include <string>
#include <vector>

// Every property of a file or folder, grouped into sections. Slow things are separate, so a window can open
// at once and fill in: ContentInfo for what is inside a file, scanFolder for what is inside a folder.
namespace Properties {

std::vector<PropertySection> collect(const std::string& path);

// Totals for several items, and the values they all share.
std::vector<PropertySection> collectSelection(const std::vector<std::string>& paths);

PropertySection folderContents(const FolderScan& scan);

// A folder's Size rows say "Counting…" until this puts in everything inside it, subfolders included.
void applyTotals(std::vector<PropertySection>& sections, const FolderScan& scan);

} // namespace Properties
