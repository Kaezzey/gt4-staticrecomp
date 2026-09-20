#include "verified_core.hpp"

#include <windows.h>
#include <bcrypt.h>
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace gt4recomp::tools {

constexpr std::size_t usa_core_size = 2020861;
constexpr auto usa_core_sha256 = "85d26aa8430154967b2633eede929286694ac39e99762527edcec365fd642ff9";

std::vector<std::uint8_t> read_verified_core(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input || input.tellg() != static_cast<std::streamoff>(usa_core_size)) {
        throw std::runtime_error("Could not read a CORE of the pinned USA v2.00 size");
    }
    input.seekg(0);
    std::vector<std::uint8_t> bytes(usa_core_size);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) {
        throw std::runtime_error("Could not read the complete CORE input");
    }
    std::array<std::uint8_t, 32> digest{};
    const auto status = BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0,
                                  bytes.data(), static_cast<ULONG>(bytes.size()),
                                  digest.data(), static_cast<ULONG>(digest.size()));
    if (status < 0) {
        throw std::runtime_error("Windows SHA-256 calculation failed");
    }
    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (const auto byte : digest) {
        hex << std::setw(2) << static_cast<unsigned>(byte);
    }
    if (hex.str() != usa_core_sha256) {
        throw std::runtime_error("CORE SHA-256 differs from the pinned USA v2.00 input");
    }
    return bytes;
}

} // namespace gt4recomp::tools
