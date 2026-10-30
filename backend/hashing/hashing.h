#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct HashResult {
    std::string name;      // "SHA-256"
    std::string value;     // lowercase hex
};

namespace Hashing {

std::vector<std::string> algorithms();                 // every name, in display order

// Text as a hash, for tests and small inputs.
std::string hashText(const std::string& algorithm, const std::string& text);

// Hashes a file with every algorithm in one pass over it. `progress` gets bytes done and bytes in total.
// Returns nothing if the file can't be read or `cancelled` said stop.
std::vector<HashResult> hashFile(const std::string& path,
                                 const std::function<void(std::uint64_t, std::uint64_t)>& progress,
                                 const std::function<bool()>& cancelled, std::string* error = nullptr);

} // namespace Hashing
