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

// Puts a file's content rows in a Content tab before Advanced; the technical ones go into Advanced.
void addContent(std::vector<PropertySection>& sections, const std::vector<PropertyRow>& rows);

// A folder's Size rows say "Counting…" until this puts in everything inside it, subfolders included.
void applyTotals(std::vector<PropertySection>& sections, const FolderScan& scan);

} // namespace Properties
