#pragma once
#include "fileops/fileops.h"

#include <functional>
#include <string>
#include <vector>

struct NetworkShare {
    std::string name;          // "files on nas"
    std::string path;          // the folder it appears as
    std::string uri;           // smb://nas/files
};

// NAS and server folders (SMB, SFTP), reached through gvfs, which makes each one appear as a normal folder.
// Passwords go to gio on its standard input, so they never show in a process list, and are not kept.
namespace NetworkShares {

std::string mountsFolder();                                        // where gvfs puts them
NetworkShare parseMountName(const std::string& folderName);        // "smb-share:server=nas,share=files"
std::vector<NetworkShare> list();
std::string normalizeAddress(const std::string& address);          // "nas/files" becomes "smb://nas/files"
Outcome connect(const std::string& uri, const std::string& user, const std::string& password,
                const std::function<bool()>& cancelled);
Outcome disconnect(const std::string& uri);

} // namespace NetworkShares
