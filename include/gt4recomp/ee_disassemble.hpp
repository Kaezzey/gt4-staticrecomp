#pragma once

#include "gt4recomp/executable_image.hpp"

#include <cstdint>
#include <iosfwd>
#include <string>

namespace gt4recomp::ee {

// Decode and format a word at its guest PC. Uses canonical instruction names,
// not aliases such as nop/move/li. Unsupported words keep their raw encoding.
[[nodiscard]] std::string format_instruction(std::uint32_t word, std::uint32_t pc);

// Linear inspection of a caller-selected, file-backed text range. Validate the
// entire range before writing; never treat a successful decode as proof of code.
// Summary and unsupported opcode counts go to a separate diagnostic stream.
void disassemble_region(const ImageRecord& text, std::uint32_t start,
                        std::uint32_t instruction_count,
                        std::ostream& listing, std::ostream& report);

} // namespace gt4recomp::ee
