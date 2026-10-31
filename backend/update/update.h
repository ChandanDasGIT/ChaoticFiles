#pragma once
#include <optional>
#include <string>

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

// A sh script. Its arguments: the update's folder, the new copy, the installed copy, what to start, then the
// program's own arguments. It takes over once the running program has quit: it swaps the copies and starts the
// new one, and if that does not start, the old one comes back.
const char* installScript();

// Files the script and the program use to tell each other what happened, in the update's folder.
inline constexpr char kOldExitedFile[] = "old-exited";              // the old program is gone
inline constexpr char kStartedFile[] = "started";                   // the new program really runs
inline constexpr char kInstallingFile[] = "installing";             // this folder is in use: don't clean it up

// Every running copy of the app keeps this lock, so an update can tell when another window is open.
bool holdInstanceLock(const std::string& path);
bool otherInstancesRunning();

} // namespace Update
