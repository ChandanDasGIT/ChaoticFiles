#pragma once
#include "properties/propertyreport.h"

#include <functional>
#include <string>
#include <vector>

// What is inside a file, by kind. Each reader returns the rows it could work out. Anything needing another
// program says so in a row when that program is missing.
namespace ContentInfo {

using Cancelled = std::function<bool()>;

std::vector<PropertyRow> collect(const std::string& path, const Cancelled& cancelled);   // picks the readers below

std::vector<PropertyRow> text(const std::string& path);
std::vector<PropertyRow> image(const std::string& path);
std::vector<PropertyRow> elf(const std::string& path);
std::vector<PropertyRow> media(const std::string& path, const Cancelled& cancelled);
std::vector<PropertyRow> archive(const std::string& path, const std::string& mime, const Cancelled& cancelled);
std::vector<PropertyRow> pdf(const std::string& path, const Cancelled& cancelled);

} // namespace ContentInfo
