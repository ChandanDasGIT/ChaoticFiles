#include "archive/archiver.h"

#include "pathbar/pathbar.h"
#include "process/process.h"

#include <algorithm>
#include <cctype>

namespace {

const char* const kTarExtensions[] = {".tar.gz", ".tar.xz", ".tar.bz2", ".tar.zst", ".tgz", ".txz", ".tbz2", ".tar"};
const char* const kOtherExtensions[] = {".zip", ".7z", ".rar", ".jar"};

std::string lowered(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
    return text;
}

bool endsWith(const std::string& text, const std::string& end)
{
    return text.size() > end.size() && text.compare(text.size() - end.size(), end.size(), end) == 0;
}

bool isTar(const std::string& name)
{
    const std::string lower = lowered(name);
    return std::any_of(std::begin(kTarExtensions), std::end(kTarExtensions), [&lower](const char* ext) { return endsWith(lower, ext); });
}

Outcome finished(const Process::Result& result, const std::string& program)
{
    if (result.cancelled) return {false, "Cancelled."};
    if (!result.started) return {false, program + " is not installed."};
    if (result.exitCode != 0) {
        std::string reason = result.errors;
        while (!reason.empty() && reason.back() == '\n') reason.pop_back();
        const auto lastLine = reason.rfind('\n');
        if (lastLine != std::string::npos) reason = reason.substr(lastLine + 1);
        return {false, program + " failed" + (reason.empty() ? "." : ": " + reason)};
    }
    return {};
}

} // namespace

namespace Archiver {

bool isArchive(const std::string& name)
{
    const std::string lower = lowered(name);
    return isTar(lower) || std::any_of(std::begin(kOtherExtensions), std::end(kOtherExtensions),
                                       [&lower](const char* ext) { return endsWith(lower, ext); });
}

std::string withoutExtension(const std::string& name)
{
    const std::string lower = lowered(name);
    for (const char* ext : kTarExtensions)
        if (endsWith(lower, ext)) return name.substr(0, name.size() - std::string(ext).size());
    for (const char* ext : kOtherExtensions)
        if (endsWith(lower, ext)) return name.substr(0, name.size() - std::string(ext).size());
    return name;
}

Outcome compress(const std::vector<std::string>& paths, const std::string& archivePath,
                 const std::function<bool()>& cancelled)
{
    if (paths.empty()) return {false, "Nothing to compress."};
    const std::string folder = PathBar::parent(paths.front());
    std::vector<std::string> names;
    // "./" stops a name that starts with a dash from being read as an option
    for (const std::string& path : paths) names.push_back("./" + PathBar::baseName(path));

    const std::string lower = lowered(archivePath);
    std::vector<std::string> command;
    std::string program;
    if (isTar(lower)) {
        program = "tar";
        command = {"tar", "-caf", archivePath};
    } else if (endsWith(lower, ".zip")) {
        program = "zip";
        command = {"zip", "-q", "-r", "-y", archivePath};
    } else if (endsWith(lower, ".7z")) {
        program = "7z";
        command = {"7z", "a", "-bd", "-y", archivePath};
    } else {
        return {false, "This kind of archive can't be created."};
    }
    command.insert(command.end(), names.begin(), names.end());

    const auto result = Process::run(command, cancelled, {}, folder);
    Outcome outcome = finished(result, program);
    if (!outcome.ok) FileOps::removeTree(archivePath, {});     // no half-made archive stays behind
    return outcome;
}

Outcome extract(const std::string& archivePath, const std::string& folder, const std::function<bool()>& cancelled)
{
    const std::string lower = lowered(archivePath);
    if (isTar(lower)) return finished(Process::run({"tar", "-xf", archivePath, "-C", folder}, cancelled), "tar");
    if (endsWith(lower, ".zip") || endsWith(lower, ".jar"))
        return finished(Process::run({"unzip", "-q", "-n", archivePath, "-d", folder}, cancelled), "unzip");
    if (endsWith(lower, ".7z") || endsWith(lower, ".rar"))
        return finished(Process::run({"7z", "x", "-bd", "-y", "-aos", "-o" + folder, "--", archivePath}, cancelled), "7z");
    return {false, "This kind of archive can't be extracted."};
}

} // namespace Archiver
