#pragma once
#include <cstdint>
#include <string>
#include <vector>

// Reading permissions: names, rwx text, ACLs and Linux capabilities.
namespace Permissions {

std::string userName(std::uint32_t uid);             // "unknown" if the system has no such user
std::string groupName(std::uint32_t gid);
std::string modeText(std::uint32_t fullMode);        // "drwxr-xr-x", as ls prints it
std::string octalText(std::uint32_t fullMode);       // "0755"
std::string whatYouCanDo(const std::string& path, bool isDirectory);

// Reads the value of system.posix_acl_access or system.posix_acl_default, as getfacl prints it.
std::vector<std::string> parseAcl(const std::string& value);

// Reads the value of security.capability: "cap_net_raw=ep"
std::string parseCapabilities(const std::string& value);

} // namespace Permissions
