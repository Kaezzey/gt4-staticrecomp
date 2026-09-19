#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace gt4recomp {

// These are guest addresses, not pointers into the host process.
struct ImageRecord {
    std::uint32_t guest_address = 0;
    std::vector<std::uint8_t> bytes;
};

struct ExecutableImage {
    std::uint32_t entry_address = 0;
    ImageRecord reginfo;
    ImageRecord text;
    ImageRecord data;
};

// Supports the observed unencrypted, raw-DEFLATE, three-record CORE format.
// Throws std::runtime_error on invalid input. Does not verify RSA or disc identity;
// the gt4core frontend pins the input hash before calling this parser.
[[nodiscard]] ExecutableImage reconstruct_core(std::span<const std::uint8_t> core);

// This named analysis policy reproduces the reference's 8 MiB zero-fill and
// reginfo relocation. Neither is claimed to describe the real runtime loader.
// The original image records are preserved; only the exported view is adjusted.
[[nodiscard]] std::vector<std::uint8_t> make_reference_analysis_elf(const ExecutableImage& image);

// Shared validation also protects export callers that construct an image directly.
void validate_image(const ExecutableImage& image);

} // namespace gt4recomp
