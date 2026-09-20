#include "gt4recomp/ee_disassemble.hpp"
#include "verified_core.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace {

std::uint32_t parse_number(std::wstring_view text) {
    std::uint32_t base = 10;
    if (text.starts_with(L"0x") || text.starts_with(L"0X")) {
        base = 16;
        text.remove_prefix(2);
    }
    if (text.empty()) {
        throw std::runtime_error("Expected decimal or 0x-prefixed hexadecimal number");
    }
    std::uint32_t result = 0;
    for (const wchar_t character : text) {
        std::uint32_t digit = 0;
        if (character >= L'0' && character <= L'9') {
            digit = character - L'0';
        } else if (character >= L'a' && character <= L'f') {
            digit = character - L'a' + 10;
        } else if (character >= L'A' && character <= L'F') {
            digit = character - L'A' + 10;
        } else {
            throw std::runtime_error("Invalid character in numeric argument");
        }
        if (digit >= base || result > (std::numeric_limits<std::uint32_t>::max() - digit) / base) {
            throw std::runtime_error("Numeric argument is invalid or exceeds 32 bits");
        }
        result = result * base + digit;
    }
    return result;
}

} // namespace

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: gt4disasm CORE.GT4 start-address instruction-count\n"
                     "Numbers: decimal or 0x-prefixed hex. Listing: stdout; opcode report: stderr.\n";
        return 2;
    }
    try {
        const auto start = parse_number(argv[2]);
        const auto count = parse_number(argv[3]);
        const auto core = gt4recomp::tools::read_verified_core(argv[1]);
        const auto image = gt4recomp::reconstruct_core(core);
        gt4recomp::ee::disassemble_region(image.text, start, count, std::cout, std::cerr);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 1;
    }
}
