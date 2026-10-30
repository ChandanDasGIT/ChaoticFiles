#include "contentinfo/contentinfo.h"

#include "process/process.h"

#include <sstream>

namespace ContentInfo {

std::vector<PropertyRow> pdf(const std::string& path, const Cancelled& cancelled)
{
    if (Process::findProgram("pdfinfo").empty()) return {{"PDF details", "Not available: pdfinfo is not installed"}};
    const auto result = Process::run({"pdfinfo", "--", path}, cancelled);
    if (result.exitCode != 0) return {{"PDF details", "pdfinfo could not read this file"}};

    // lines look like "Title:          My report"
    std::vector<PropertyRow> rows;
    std::istringstream lines(result.output);
    std::string line;
    while (std::getline(lines, line)) {
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        const auto start = line.find_first_not_of(' ', colon + 1);
        if (start != std::string::npos) rows.push_back({line.substr(0, colon), line.substr(start)});
    }
    return rows;
}

} // namespace ContentInfo
