#include "hashing/hashing.h"

#include "blake3.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>

namespace {

constexpr std::uint32_t rotl32(std::uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }
constexpr std::uint32_t rotr32(std::uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }
constexpr std::uint64_t rotr64(std::uint64_t x, int n) { return (x >> n) | (x << (64 - n)); }

std::string toHex(const unsigned char* bytes, std::size_t size)
{
    static const char digits[] = "0123456789abcdef";
    std::string text;
    text.reserve(size * 2);
    for (std::size_t i = 0; i < size; ++i) {
        text += digits[bytes[i] >> 4];
        text += digits[bytes[i] & 15];
    }
    return text;
}

class Hasher {
public:
    virtual ~Hasher() = default;
    virtual void update(const unsigned char* data, std::size_t size) = 0;
    virtual std::string finish() = 0;
};

// The block-based hashes: collects input into blocks, then pads with 0x80, zeros and the bit length.
template <std::size_t BlockSize, std::size_t LengthSize, bool BigEndianLength>
class BlockHasher : public Hasher {
public:
    void update(const unsigned char* data, std::size_t size) override
    {
        m_total += size;
        while (size > 0) {
            if (m_used == 0 && size >= BlockSize) {
                block(data);
                data += BlockSize;
                size -= BlockSize;
                continue;
            }
            const std::size_t take = std::min(size, BlockSize - m_used);
            std::memcpy(m_buffer.data() + m_used, data, take);
            m_used += take;
            data += take;
            size -= take;
            if (m_used == BlockSize) {
                block(m_buffer.data());
                m_used = 0;
            }
        }
    }

protected:
    virtual void block(const unsigned char* data) = 0;

    void pad()
    {
        const std::uint64_t bits = m_total * 8;
        const unsigned char one = 0x80, zero = 0;
        update(&one, 1);
        while (m_used != BlockSize - LengthSize) update(&zero, 1);
        unsigned char length[LengthSize] = {};
        for (std::size_t i = 0; i < 8; ++i) {
            const auto byte = static_cast<unsigned char>(bits >> (8 * i));
            length[BigEndianLength ? LengthSize - 1 - i : i] = byte;
        }
        update(length, LengthSize);
    }

private:
    std::array<unsigned char, BlockSize> m_buffer {};
    std::size_t m_used = 0;
    std::uint64_t m_total = 0;
};

class Md5 : public BlockHasher<64, 8, false> {
public:
    std::string finish() override
    {
        pad();
        unsigned char out[16];
        for (int i = 0; i < 4; ++i)
            for (int b = 0; b < 4; ++b) out[i * 4 + b] = static_cast<unsigned char>(m_state[i] >> (8 * b));
        return toHex(out, sizeof out);
    }

protected:
    void block(const unsigned char* data) override
    {
        static const std::uint32_t K[64] = {
            0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
            0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
            0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
            0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
            0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
            0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
            0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
            0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};
        static const int S[16] = {7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21};

        std::uint32_t M[16];
        std::memcpy(M, data, 64);          // x86-64 is little endian, as MD5 wants
        std::uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3];
        for (int i = 0; i < 64; ++i) {
            std::uint32_t f;
            int g;
            if (i < 16) { f = (b & c) | (~b & d); g = i; }
            else if (i < 32) { f = (d & b) | (~d & c); g = (5 * i + 1) % 16; }
            else if (i < 48) { f = b ^ c ^ d; g = (3 * i + 5) % 16; }
            else { f = c ^ (b | ~d); g = (7 * i) % 16; }
            const std::uint32_t next = d;
            d = c;
            c = b;
            b += rotl32(a + f + K[i] + M[g], S[i / 16 * 4 + i % 4]);
            a = next;
        }
        m_state[0] += a; m_state[1] += b; m_state[2] += c; m_state[3] += d;
    }

private:
    std::uint32_t m_state[4] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
};

class Sha1 : public BlockHasher<64, 8, true> {
public:
    std::string finish() override
    {
        pad();
        unsigned char out[20];
        for (int i = 0; i < 5; ++i)
            for (int b = 0; b < 4; ++b) out[i * 4 + b] = static_cast<unsigned char>(m_state[i] >> (24 - 8 * b));
        return toHex(out, sizeof out);
    }

protected:
    void block(const unsigned char* data) override
    {
        std::uint32_t w[80];
        for (int i = 0; i < 16; ++i)
            w[i] = std::uint32_t(data[i * 4]) << 24 | data[i * 4 + 1] << 16 | data[i * 4 + 2] << 8 | data[i * 4 + 3];
        for (int i = 16; i < 80; ++i) w[i] = rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

        std::uint32_t a = m_state[0], b = m_state[1], c = m_state[2], d = m_state[3], e = m_state[4];
        for (int i = 0; i < 80; ++i) {
            std::uint32_t f, k;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5a827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ed9eba1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8f1bbcdc; }
            else { f = b ^ c ^ d; k = 0xca62c1d6; }
            const std::uint32_t next = rotl32(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rotl32(b, 30); b = a; a = next;
        }
        m_state[0] += a; m_state[1] += b; m_state[2] += c; m_state[3] += d; m_state[4] += e;
    }

private:
    std::uint32_t m_state[5] = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
};

class Sha256 : public BlockHasher<64, 8, true> {
public:
    std::string finish() override
    {
        pad();
        unsigned char out[32];
        for (int i = 0; i < 8; ++i)
            for (int b = 0; b < 4; ++b) out[i * 4 + b] = static_cast<unsigned char>(m_state[i] >> (24 - 8 * b));
        return toHex(out, sizeof out);
    }

protected:
    void block(const unsigned char* data) override
    {
        static const std::uint32_t K[64] = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

        std::uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = std::uint32_t(data[i * 4]) << 24 | data[i * 4 + 1] << 16 | data[i * 4 + 2] << 8 | data[i * 4 + 3];
        for (int i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotr32(w[i - 15], 7) ^ rotr32(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const std::uint32_t s1 = rotr32(w[i - 2], 17) ^ rotr32(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        std::uint32_t v[8];
        std::memcpy(v, m_state, sizeof v);
        for (int i = 0; i < 64; ++i) {
            const std::uint32_t s1 = rotr32(v[4], 6) ^ rotr32(v[4], 11) ^ rotr32(v[4], 25);
            const std::uint32_t ch = (v[4] & v[5]) ^ (~v[4] & v[6]);
            const std::uint32_t t1 = v[7] + s1 + ch + K[i] + w[i];
            const std::uint32_t s0 = rotr32(v[0], 2) ^ rotr32(v[0], 13) ^ rotr32(v[0], 22);
            const std::uint32_t maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
            const std::uint32_t t2 = s0 + maj;
            v[7] = v[6]; v[6] = v[5]; v[5] = v[4]; v[4] = v[3] + t1;
            v[3] = v[2]; v[2] = v[1]; v[1] = v[0]; v[0] = t1 + t2;
        }
        for (int i = 0; i < 8; ++i) m_state[i] += v[i];
    }

private:
    std::uint32_t m_state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
};

class Sha512 : public BlockHasher<128, 16, true> {
public:
    std::string finish() override
    {
        pad();
        unsigned char out[64];
        for (int i = 0; i < 8; ++i)
            for (int b = 0; b < 8; ++b) out[i * 8 + b] = static_cast<unsigned char>(m_state[i] >> (56 - 8 * b));
        return toHex(out, sizeof out);
    }

protected:
    void block(const unsigned char* data) override
    {
        static const std::uint64_t K[80] = {
            0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc, 0x3956c25bf348b538,
            0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118, 0xd807aa98a3030242, 0x12835b0145706fbe,
            0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2, 0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235,
            0xc19bf174cf692694, 0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
            0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5, 0x983e5152ee66dfab,
            0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4, 0xc6e00bf33da88fc2, 0xd5a79147930aa725,
            0x06ca6351e003826f, 0x142929670a0e6e70, 0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed,
            0x53380d139d95b3df, 0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
            0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30, 0xd192e819d6ef5218,
            0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8, 0x19a4c116b8d2d0c8, 0x1e376c085141ab53,
            0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8, 0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373,
            0x682e6ff3d6b2b8a3, 0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
            0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b, 0xca273eceea26619c,
            0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178, 0x06f067aa72176fba, 0x0a637dc5a2c898a6,
            0x113f9804bef90dae, 0x1b710b35131c471b, 0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc,
            0x431d67c49c100d4c, 0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817};

        std::uint64_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = 0;
            for (int b = 0; b < 8; ++b) w[i] = (w[i] << 8) | data[i * 8 + b];
        }
        for (int i = 16; i < 80; ++i) {
            const std::uint64_t s0 = rotr64(w[i - 15], 1) ^ rotr64(w[i - 15], 8) ^ (w[i - 15] >> 7);
            const std::uint64_t s1 = rotr64(w[i - 2], 19) ^ rotr64(w[i - 2], 61) ^ (w[i - 2] >> 6);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        std::uint64_t v[8];
        std::memcpy(v, m_state, sizeof v);
        for (int i = 0; i < 80; ++i) {
            const std::uint64_t s1 = rotr64(v[4], 14) ^ rotr64(v[4], 18) ^ rotr64(v[4], 41);
            const std::uint64_t ch = (v[4] & v[5]) ^ (~v[4] & v[6]);
            const std::uint64_t t1 = v[7] + s1 + ch + K[i] + w[i];
            const std::uint64_t s0 = rotr64(v[0], 28) ^ rotr64(v[0], 34) ^ rotr64(v[0], 39);
            const std::uint64_t maj = (v[0] & v[1]) ^ (v[0] & v[2]) ^ (v[1] & v[2]);
            const std::uint64_t t2 = s0 + maj;
            v[7] = v[6]; v[6] = v[5]; v[5] = v[4]; v[4] = v[3] + t1;
            v[3] = v[2]; v[2] = v[1]; v[1] = v[0]; v[0] = t1 + t2;
        }
        for (int i = 0; i < 8; ++i) m_state[i] += v[i];
    }

private:
    std::uint64_t m_state[8] = {0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
                                0x510e527fade682d1, 0x9b05688c2b3e6c1f, 0x1f83d9abfb41bd6b, 0x5be0cd19137e2179};
};

class Blake3 : public Hasher {
public:
    Blake3() { blake3_hasher_init(&m_hasher); }
    void update(const unsigned char* data, std::size_t size) override { blake3_hasher_update(&m_hasher, data, size); }
    std::string finish() override
    {
        unsigned char out[BLAKE3_OUT_LEN];
        blake3_hasher_finalize(&m_hasher, out, sizeof out);
        return toHex(out, sizeof out);
    }

private:
    blake3_hasher m_hasher;
};

std::unique_ptr<Hasher> makeHasher(const std::string& name)
{
    if (name == "MD5") return std::make_unique<Md5>();
    if (name == "SHA-1") return std::make_unique<Sha1>();
    if (name == "SHA-256") return std::make_unique<Sha256>();
    if (name == "SHA-512") return std::make_unique<Sha512>();
    if (name == "BLAKE3") return std::make_unique<Blake3>();
    return nullptr;
}

} // namespace

namespace Hashing {

std::vector<std::string> algorithms()
{
    return {"MD5", "SHA-1", "SHA-256", "SHA-512", "BLAKE3"};
}

std::string hashText(const std::string& algorithm, const std::string& text)
{
    auto hasher = makeHasher(algorithm);
    if (!hasher) return {};
    hasher->update(reinterpret_cast<const unsigned char*>(text.data()), text.size());
    return hasher->finish();
}

std::vector<HashResult> hashFile(const std::string& path,
                                 const std::function<void(std::uint64_t, std::uint64_t)>& progress,
                                 const std::function<bool()>& cancelled, std::string* error)
{
    const int fd = open(path.c_str(), O_RDONLY | O_CLOEXEC);
    struct stat info {};
    if (fd < 0 || fstat(fd, &info) != 0) {
        if (error) *error = std::strerror(errno);
        if (fd >= 0) close(fd);
        return {};
    }

    std::vector<std::pair<std::string, std::unique_ptr<Hasher>>> hashers;
    for (const std::string& name : algorithms()) hashers.emplace_back(name, makeHasher(name));

    std::vector<unsigned char> buffer(1024 * 1024);
    const auto total = static_cast<std::uint64_t>(info.st_size);
    std::uint64_t done = 0;
    while (true) {
        if (cancelled && cancelled()) {
            close(fd);
            return {};
        }
        const ssize_t got = read(fd, buffer.data(), buffer.size());
        if (got < 0) {
            if (error) *error = std::strerror(errno);
            close(fd);
            return {};
        }
        if (got == 0) break;
        for (auto& entry : hashers) entry.second->update(buffer.data(), static_cast<std::size_t>(got));
        done += static_cast<std::uint64_t>(got);
        if (progress) progress(done, total);
    }
    close(fd);

    std::vector<HashResult> results;
    for (auto& entry : hashers) results.push_back({entry.first, entry.second->finish()});
    return results;
}

} // namespace Hashing
