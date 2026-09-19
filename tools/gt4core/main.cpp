#include "gt4recomp/executable_image.hpp"

#include <windows.h>
#include <bcrypt.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

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

void write_new_file(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes) {
    // CREATE_NEW refuses existing files atomically, including an input/output alias.
    const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Cannot create output; parent must exist and output must be new");
    }
    DWORD written = 0;
    const bool succeeded = WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr);
    const bool closed = CloseHandle(file);
    if (!succeeded || !closed || written != bytes.size()) {
        throw std::runtime_error("Could not write the complete ELF; remove the partial output before retrying");
    }
}

void describe(const char* name, const gt4recomp::ImageRecord& record) {
    std::cout << name << ": guest=0x" << std::hex << record.guest_address
              << " bytes=0x" << record.bytes.size() << std::dec << '\n';
}

} // namespace

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 4 || std::wstring(argv[1]) != L"--reference-analysis") {
        std::cerr << "Usage: gt4core --reference-analysis CORE.GT4 output.elf\n"
                  << "Analysis only: explicitly adopts the reference's unproven BSS/reginfo policy.\n";
        return 2;
    }
    try {
        const auto core = read_verified_core(argv[2]);
        const auto image = gt4recomp::reconstruct_core(core);
        const auto elf = gt4recomp::make_reference_analysis_elf(image);
        write_new_file(argv[3], elf);
        std::cout << "Pinned USA v2.00 CORE reconstructed; entry=0x" << std::hex
                  << image.entry_address << std::dec << '\n';
        describe("CORE reginfo (original address)", image.reginfo);
        describe("text", image.text);
        describe("data", image.data);
        std::cout << "Analysis policy: reginfo +24 bytes; data zero-fill 0x800000 bytes.\n"
                  << "These are reference-tool choices, not verified runtime behavior.\n"
                  << "Wrote " << elf.size() << " bytes.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
