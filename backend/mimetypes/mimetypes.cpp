#include "mimetypes/mimetypes.h"
#include "pathbar/pathbar.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fnmatch.h>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <unordered_map>

namespace {

struct Glob {
    std::string type;
    std::string pattern;
    int weight = 50;
    bool caseSensitive = false;
};

// What the list gets wrong for programmers: it knows *.ts only as Qt Linguist and video, and *.tsx as Tiled.
const std::map<std::string, std::string> kCorrections = {
    {"text/vnd.trolltech.linguist:*.ts", "text/x-typescript"},
    {"application/x-tiled-tsx:*.tsx", "text/x-typescript-jsx"},
};

struct Database {
    std::unordered_map<std::string, std::vector<Glob>> names;       // whole names, in lower case
    std::unordered_map<std::string, std::vector<Glob>> extensions;  // "tar.gz", in lower case
    std::vector<Glob> patterns;                                     // anything else
    std::unordered_map<std::string, std::string> aliases;           // alias, real name
    std::unordered_map<std::string, std::vector<std::string>> parents;
    std::vector<std::string> folders;
    std::mutex descriptionLock;
    std::unordered_map<std::string, std::string> descriptions;
};

std::string lower(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
    return text;
}

bool plainText(const std::string& text)
{
    return text.find_first_of("*?[") == std::string::npos;
}

// weight:type:pattern, and sometimes :cs for case sensitive
void readGlobs(const std::string& path, Database& data)
{
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        const auto first = line.find(':');
        const auto second = first == std::string::npos ? first : line.find(':', first + 1);
        if (second == std::string::npos) continue;
        const auto third = line.find(':', second + 1);

        Glob glob;
        glob.weight = std::atoi(line.c_str());
        glob.type = line.substr(first + 1, second - first - 1);
        glob.pattern = line.substr(second + 1, third == std::string::npos ? third : third - second - 1);
        glob.caseSensitive = third != std::string::npos && line.find("cs", third) != std::string::npos;

        if (const auto fix = kCorrections.find(glob.type + ":" + glob.pattern); fix != kCorrections.end()) glob.type = fix->second;
        if (plainText(glob.pattern)) data.names[lower(glob.pattern)].push_back(glob);
        else if (glob.pattern.rfind("*.", 0) == 0 && plainText(glob.pattern.substr(2)))
            data.extensions[lower(glob.pattern.substr(2))].push_back(glob);
        else data.patterns.push_back(glob);
    }
}

// lines with two words
template <typename Use>
void readPairs(const std::string& path, char separator, const Use& use)
{
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        const auto split = line.find(separator);
        if (line.empty() || line[0] == '#' || split == std::string::npos) continue;
        use(line.substr(0, split), line.substr(split + 1));
    }
}

std::vector<std::string> systemFolders()
{
    std::vector<std::string> folders;
    const char* home = std::getenv("XDG_DATA_HOME");
    folders.push_back(home && *home ? home : PathBar::homeDirectory() + "/.local/share");

    const char* dirs = std::getenv("XDG_DATA_DIRS");
    std::stringstream list(dirs && *dirs ? dirs : "/usr/local/share:/usr/share");
    for (std::string folder; std::getline(list, folder, ':');)
        if (!folder.empty()) folders.push_back(folder);
    return folders;
}

std::shared_ptr<Database> load(const std::vector<std::string>& folders)
{
    auto data = std::make_shared<Database>();
    data->folders = folders;
    for (const std::string& folder : folders) {
        const std::string mime = folder + "/mime/";
        readGlobs(mime + "globs2", *data);
        readPairs(mime + "aliases", ' ', [&](const std::string& alias, const std::string& type) { data->aliases.emplace(alias, type); });
        readPairs(mime + "subclasses", ' ', [&](const std::string& type, const std::string& parent) { data->parents[type].push_back(parent); });
    }
    return data;
}

std::mutex databaseLock;
std::shared_ptr<Database> database;

std::shared_ptr<Database> data()
{
    std::lock_guard guard(databaseLock);
    if (!database) database = load(systemFolders());
    return database;
}

// The heaviest of these that really matches, and an exact match of case beats one that ignores it.
// A type can be listed in more than one folder.
void keepBest(const std::vector<Glob>& globs, const std::string& wanted, std::vector<std::string>& found)
{
    const bool exact = std::any_of(globs.begin(), globs.end(), [&wanted](const Glob& glob) { return glob.caseSensitive && glob.pattern == wanted; });
    const auto counts = [&](const Glob& glob) { return exact ? glob.caseSensitive && glob.pattern == wanted : !glob.caseSensitive; };
    int best = 0;
    for (const Glob& glob : globs)
        if (counts(glob) && glob.weight > best) best = glob.weight;
    for (const Glob& glob : globs)
        if (counts(glob) && glob.weight == best && std::find(found.begin(), found.end(), glob.type) == found.end())
            found.push_back(glob.type);
}

std::string canonical(const Database& data, const std::string& type)
{
    const auto it = data.aliases.find(type);
    return it != data.aliases.end() ? it->second : type;
}

} // namespace

namespace MimeTypes {

void useFolders(const std::vector<std::string>& dataFolders)
{
    std::lock_guard guard(databaseLock);
    database = load(dataFolders);
}

// *.json is JSON and also a JSON schema, which is a kind of JSON: that is one answer, not two
static std::vector<std::string> withoutKinds(const std::vector<std::string>& types)
{
    std::vector<std::string> kept;
    for (const std::string& type : types) {
        const std::vector<std::string> kinds = chain(type);
        const bool kindOfAnother = std::any_of(types.begin(), types.end(), [&](const std::string& other) {
            return other != type && std::find(kinds.begin(), kinds.end(), other) != kinds.end();
        });
        if (!kindOfAnother) kept.push_back(type);
    }
    return kept;
}

static std::vector<std::string> matches(const std::string& fileName)
{
    const auto list = data();
    std::vector<std::string> found;

    if (const auto it = list->names.find(lower(fileName)); it != list->names.end()) keepBest(it->second, fileName, found);
    if (!found.empty()) return found;

    // the longest extension wins: archive.tar.gz is a tar.gz before it is a gz
    for (auto dot = fileName.find('.'); dot != std::string::npos; dot = fileName.find('.', dot + 1)) {
        const std::string extension = fileName.substr(dot + 1);
        if (const auto it = list->extensions.find(lower(extension)); it != list->extensions.end())
            keepBest(it->second, "*." + extension, found);
        if (!found.empty()) return found;
    }

    std::vector<Glob> matching;
    for (const Glob& glob : list->patterns)
        if (fnmatch(glob.pattern.c_str(), fileName.c_str(), glob.caseSensitive ? 0 : FNM_CASEFOLD) == 0) matching.push_back(glob);
    int best = 0;
    for (const Glob& glob : matching) best = std::max(best, glob.weight);
    for (const Glob& glob : matching)
        if (glob.weight == best && std::find(found.begin(), found.end(), glob.type) == found.end()) found.push_back(glob.type);
    return found;
}

std::vector<std::string> candidates(const std::string& fileName)
{
    return withoutKinds(matches(fileName));
}

std::string fromName(const std::string& fileName)
{
    const std::vector<std::string> found = candidates(fileName);
    return found.size() == 1 ? found.front() : std::string();
}

bool isExactName(const std::string& fileName)
{
    const auto list = data();
    return list->names.count(lower(fileName)) != 0;
}

std::vector<std::string> chain(const std::string& mimeType)
{
    const auto list = data();
    std::vector<std::string> types {canonical(*list, mimeType)};
    for (std::size_t at = 0; at < types.size(); ++at) {
        const auto it = list->parents.find(types[at]);
        if (it == list->parents.end()) continue;
        for (const std::string& parent : it->second) {
            const std::string type = canonical(*list, parent);
            // scripts are listed as programs, and that says nothing about which app opens them
            if (type == "application/x-executable" || type == "application/octet-stream") continue;
            if (std::find(types.begin(), types.end(), type) == types.end()) types.push_back(type);
        }
    }
    if (types.front().rfind("text/", 0) == 0 && std::find(types.begin(), types.end(), "text/plain") == types.end())
        types.push_back("text/plain");
    return types;
}

std::vector<std::string> related(const std::string& mimeType)
{
    const auto list = data();
    std::vector<std::string> types = chain(mimeType);
    if (std::find(types.begin(), types.end(), mimeType) == types.end()) types.push_back(mimeType);
    for (const auto& [alias, type] : list->aliases)
        if (std::find(types.begin(), types.end(), type) != types.end()) types.push_back(alias);
    return types;
}

std::vector<std::string> names(const std::string& mimeType)
{
    const auto list = data();
    const std::string real = canonical(*list, mimeType);
    std::vector<std::string> types {real};
    if (mimeType != real) types.push_back(mimeType);
    for (const auto& [alias, type] : list->aliases)
        if (type == real && alias != mimeType) types.push_back(alias);
    return types;
}

std::string description(const std::string& mimeType)
{
    const auto list = data();
    const std::string type = canonical(*list, mimeType);
    std::lock_guard guard(list->descriptionLock);
    if (const auto it = list->descriptions.find(type); it != list->descriptions.end()) return it->second;

    std::string text;
    for (const std::string& folder : list->folders) {
        std::ifstream file(folder + "/mime/" + type + ".xml");
        if (!file) continue;
        std::stringstream content;
        content << file.rdbuf();
        const std::string xml = content.str();
        const std::string open = "<comment>";                       // the translations have an xml:lang
        const auto start = xml.find(open);
        const auto end = start == std::string::npos ? start : xml.find("</comment>", start);
        if (end == std::string::npos) continue;
        text = xml.substr(start + open.size(), end - start - open.size());
        for (std::size_t at = text.find("&amp;"); at != std::string::npos; at = text.find("&amp;", at + 1)) text.replace(at, 5, "&");
        break;
    }
    list->descriptions.emplace(type, text);
    return text;
}

} // namespace MimeTypes
