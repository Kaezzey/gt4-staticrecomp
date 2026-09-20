#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace gt4recomp::ee {

enum class Operation {
    Unsupported,
    Addiu, Addu, Subu, And, Or, Xor, Lui, Lw, Sw,
    Beq, Bne, J, Jal, Jr, Sll, Srl, Andi, Ori
};

// These are overlapping views of the encoded bits, not a list of operands.
// For example, rd and shift_amount are not operands of an I-format instruction.
struct DecodedInstruction {
    Operation operation = Operation::Unsupported;
    std::uint32_t word = 0;
    std::uint8_t primary_opcode = 0;
    std::uint8_t rs = 0;
    std::uint8_t rt = 0;
    std::uint8_t rd = 0;
    std::uint8_t shift_amount = 0;
    std::uint8_t function = 0;
    std::uint16_t immediate = 0;
    std::uint32_t jump_index = 0;

    // Interpret the immediate as signed without narrowing to a signed 16-bit type.
    [[nodiscard]] std::int32_t signed_immediate() const;
};

// The fixed extent requires exactly four bytes. Callers check buffer bounds.
[[nodiscard]] std::uint32_t read_instruction_word(
    std::span<const std::uint8_t, 4> bytes);
[[nodiscard]] DecodedInstruction decode(std::uint32_t word);
[[nodiscard]] std::string_view mnemonic(Operation operation);

} // namespace gt4recomp::ee
