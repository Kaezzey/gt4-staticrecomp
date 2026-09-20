#include "gt4recomp/executable_image.hpp"
#include "verified_core.hpp"

#include <windows.h>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

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
        const auto core = gt4recomp::tools::read_verified_core(argv[2]);
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
