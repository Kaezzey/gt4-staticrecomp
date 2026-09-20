#include "gt4recomp/ee_disassemble.hpp"
#include "gt4recomp/ee_decode.hpp"

#include <array>
#include <iomanip>
#include <map>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace gt4recomp::ee {
namespace {

constexpr std::array<std::string_view, 32> register_names = {
    "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

std::string hex_value(std::uint32_t value, int width = 0) {
    std::ostringstream output;
    output << std::hex << std::setfill('0') << std::setw(width) << value;
    return output.str();
}

std::string signed_hex(std::int32_t value) {
    if (value < 0) {
        // Widen before negation so even INT32_MIN has a representable magnitude.
        const auto magnitude = static_cast<std::uint32_t>(-static_cast<std::int64_t>(value));
        return "-0x" + hex_value(magnitude);
    }
    return "0x" + hex_value(static_cast<std::uint32_t>(value));
}

std::string unsupported_family(const DecodedInstruction& instruction) {
    std::string family = "opcode=0x" + hex_value(instruction.primary_opcode, 2);
    if (instruction.primary_opcode == 0 || instruction.primary_opcode == 0x1c) {
        family += " function=0x" + hex_value(instruction.function, 2);
    } else if (instruction.primary_opcode == 1) {
        family += " rt=0x" + hex_value(instruction.rt, 2);
    } else if (instruction.primary_opcode >= 0x10 && instruction.primary_opcode <= 0x12) {
        family += " rs=0x" + hex_value(instruction.rs, 2);
    }
    return family;
}

} // namespace

std::string format_instruction(std::uint32_t word, std::uint32_t pc) {
    const auto instruction = decode(word);
    std::ostringstream output;
    output << mnemonic(instruction.operation);
    const auto rs = register_names[instruction.rs];
    const auto rt = register_names[instruction.rt];
    const auto rd = register_names[instruction.rd];
    switch (instruction.operation) {
    case Operation::Addu:
    case Operation::Subu:
    case Operation::And:
    case Operation::Or:
    case Operation::Xor:
        output << ' ' << rd << ", " << rs << ", " << rt;
        break;
    case Operation::Addiu:
        output << ' ' << rt << ", " << rs << ", " << signed_hex(instruction.signed_immediate());
        break;
    case Operation::Lui:
        output << ' ' << rt << ", 0x" << hex_value(instruction.immediate);
        break;
    case Operation::Andi:
    case Operation::Ori:
        output << ' ' << rt << ", " << rs << ", 0x" << hex_value(instruction.immediate);
        break;
    case Operation::Lw:
    case Operation::Sw:
        output << ' ' << rt << ", " << signed_hex(instruction.signed_immediate()) << '(' << rs << ')';
        break;
    case Operation::Beq:
    case Operation::Bne: {
        // Multiplication, not a left shift of a negative signed integer.
        // Conversion back to uint32_t explicitly wraps the guest address.
        const std::int64_t target = static_cast<std::int64_t>(pc) + 4
            + static_cast<std::int64_t>(instruction.signed_immediate()) * 4;
        output << ' ' << rs << ", " << rt << ", 0x"
               << hex_value(static_cast<std::uint32_t>(target), 8);
        break;
    }
    case Operation::J:
    case Operation::Jal: {
        const std::uint32_t next_pc = pc + std::uint32_t{4};
        const auto target = (next_pc & 0xf0000000u) | (instruction.jump_index << 2);
        output << " 0x" << hex_value(target, 8);
        break;
    }
    case Operation::Jr:
        output << ' ' << rs;
        break;
    case Operation::Sll:
    case Operation::Srl:
        output << ' ' << rd << ", " << rt << ", 0x" << hex_value(instruction.shift_amount);
        break;
    case Operation::Unsupported:
        output << " 0x" << hex_value(word, 8) << " ; " << unsupported_family(instruction);
        break;
    }
    return output.str();
}

void disassemble_region(const ImageRecord& text, std::uint32_t start,
                        std::uint32_t instruction_count,
                        std::ostream& listing, std::ostream& report) {
    const std::uint64_t text_end = static_cast<std::uint64_t>(text.guest_address) + text.bytes.size();
    const std::uint64_t end = static_cast<std::uint64_t>(start)
        + static_cast<std::uint64_t>(instruction_count) * 4;
    if (instruction_count == 0 || start % 4 != 0 || text.guest_address % 4 != 0
        || start < text.guest_address || end > text_end || text_end > 0x100000000ull) {
        throw std::runtime_error("Expected a nonempty aligned range wholly inside file-backed text");
    }
    std::map<std::string, std::uint32_t> unsupported_counts;
    std::uint32_t unsupported_total = 0;
    for (std::uint32_t index = 0; index < instruction_count; ++index) {
        const auto pc = static_cast<std::uint32_t>(static_cast<std::uint64_t>(start) + index * 4ull);
        const auto offset = static_cast<std::size_t>(pc - text.guest_address);
        const auto bytes = std::span<const std::uint8_t, 4>(text.bytes.data() + offset, 4);
        const auto word = read_instruction_word(bytes);
        const auto instruction = decode(word);
        listing << hex_value(pc, 8) << ": " << hex_value(word, 8) << "  "
                << format_instruction(word, pc) << '\n';
        if (instruction.operation == Operation::Unsupported) {
            ++unsupported_total;
            ++unsupported_counts[unsupported_family(instruction)];
        }
    }
    report << "region=0x" << hex_value(start, 8) << " words=" << instruction_count
           << " supported=" << instruction_count - unsupported_total
           << " unsupported=" << unsupported_total << '\n';
    for (const auto& [family, count] : unsupported_counts) {
        report << "unsupported " << family << " count=" << count << '\n';
    }
    if (!listing || !report) {
        throw std::runtime_error("Could not write the complete disassembly/report");
    }
}

} // namespace gt4recomp::ee
