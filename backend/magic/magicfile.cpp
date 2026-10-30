#include "magic/magicfile.h"

#include <dlfcn.h>
#include <mutex>

namespace {

constexpr int kMimeType = 0x000010, kMimeEncoding = 0x000400, kNoFollowNone = 0;

using Open = void* (*)(int);
using Load = int (*)(void*, const char*);
using File = const char* (*)(void*, const char*);
using SetFlags = int (*)(void*, int);

struct Library {
    File file = nullptr;
    SetFlags setFlags = nullptr;
    void* cookie = nullptr;
};

Library& library()
{
    static Library loaded = [] {
        Library lib;
        void* handle = dlopen("libmagic.so.1", RTLD_NOW | RTLD_LOCAL);
        if (!handle) return lib;
        auto open = reinterpret_cast<Open>(dlsym(handle, "magic_open"));
        auto load = reinterpret_cast<Load>(dlsym(handle, "magic_load"));
        lib.file = reinterpret_cast<File>(dlsym(handle, "magic_file"));
        lib.setFlags = reinterpret_cast<SetFlags>(dlsym(handle, "magic_setflags"));
        if (!open || !load || !lib.file || !lib.setFlags) return Library {};
        lib.cookie = open(kNoFollowNone);
        if (!lib.cookie || load(lib.cookie, nullptr) != 0) return Library {};
        return lib;
    }();
    return loaded;
}

std::string ask(const std::string& path, int flags)
{
    static std::mutex lock;                 // one libmagic cookie can't be used by two threads at once
    std::lock_guard guard(lock);
    Library& lib = library();
    if (!lib.cookie) return {};
    lib.setFlags(lib.cookie, flags);
    const char* answer = lib.file(lib.cookie, path.c_str());
    return answer ? answer : "";
}

} // namespace

namespace MagicFile {

bool available() { return library().cookie != nullptr; }
std::string describe(const std::string& path) { return ask(path, 0); }
std::string mimeType(const std::string& path) { return ask(path, kMimeType); }
std::string encoding(const std::string& path) { return ask(path, kMimeEncoding); }

} // namespace MagicFile
