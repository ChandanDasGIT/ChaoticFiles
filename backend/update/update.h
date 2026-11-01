#pragma once
#include <optional>
#include <string>
#include <vector>

// The rules of an update, with no network and no Qt: which version is newer, whether a manifest is signed and
// well formed, and the script that swaps the program. See UpdateController for how they are used.
namespace Update {

// The signed list of a release's files.
//   ChaoticFiles update manifest 1
//   version 1.1
//   tarball ChaoticFiles-1.1-linux-x86_64.tar.xz <sha256>
//   appimage ChaoticFiles-1.1-x86_64.AppImage <sha256>
struct Manifest {
    std::string version;
    std::string tarball;
    std::string tarballHash;
    std::string appImage;
    std::string appImageHash;
};

bool isVersion(const std::string& text);                           // digits and dots only: 1.1, 2.0.3
bool isNewer(const std::string& version, const std::string& running);
std::optional<Manifest> parseManifest(const std::string& text);     // nothing unless every line is exactly right

// The public key is 64 hex digits and the signature is the raw 64 bytes, of Ed25519.
bool signatureIsValid(const std::string& message, const std::string& signature, const std::string& publicKeyHex);

// A sh script. Its arguments: the update's folder, the new copy, the installed copy, where the new copy goes, what
// starts the new version, what starts the old one, then the program's own arguments. It takes over once the running
// program has quit: it swaps the copies and starts the new one, and if that does not start, the old one comes back.
const char* installScript();

// Where the new AppImage goes: its own name beside the old one, so it never needs renaming. A name the user chose
// is not a release's name, and it stays, so that their shortcuts keep working.
std::string appImageTarget(const std::string& installedPath, const std::string& oldVersion, const std::string& newName);

// Files the script and the program use to tell each other what happened, in the update's folder.
inline constexpr char kOldExitedFile[] = "old-exited";              // the old program is gone
inline constexpr char kStartedFile[] = "started";                   // the new program really runs
inline constexpr char kInstallingFile[] = "installing";             // this folder is in use: don't clean it up

// Every running copy of the app keeps this lock, so an update can tell when another window is open.
bool holdInstanceLock(const std::string& path);
bool otherInstancesRunning();

// Closing every window for an update. The window that restarts keeps asking, in a file in `folder`, and the others
// answer by leaving their tabs there and quitting. A request that nobody renews runs out, so a crash can't close
// windows later on.
inline constexpr int kCloseRequestLifetimeSeconds = 10;
void askWindowsToClose(const std::string& folder, int pid);          // again every second or so, for as long as it waits
void stopAskingWindowsToClose(const std::string& folder, int pid);   // does nothing if another window took over asking
int closeRequester(const std::string& folder);                       // the pid that is asking, or 0 if none is

void rememberWindow(const std::string& folder, int pid, const std::vector<std::string>& arguments);
std::vector<std::vector<std::string>> takeRememberedWindows(const std::string& folder);   // each once, even with two callers

} // namespace Update
