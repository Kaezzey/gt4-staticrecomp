#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

namespace gt4recomp::tools {

// Read and hash-check the one USA v2.00 CORE accepted by the native tools.
[[nodiscard]] std::vector<std::uint8_t> read_verified_core(const std::filesystem::path& path);

} // namespace gt4recomp::tools
