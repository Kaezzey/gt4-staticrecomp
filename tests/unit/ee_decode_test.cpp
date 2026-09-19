#include "gt4recomp/ee_decode.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <string_view>

using namespace gt4recomp::ee;

int main() {
    int failures = 0;
    const auto check = [&](bool passed, std::uint32_t word, std::string_view detail) {
        if (!passed) {
            std::cerr << "word 0x" << std::hex << word << ": " << detail << '\n';
            ++failures;
        }
    };

    // Literal words and expected operands were selected by hand, not encoded
    // by a helper that could share a field-position bug with the decoder.
    struct RegisterCase {
        std::uint32_t word;
        Operation operation;
        std::string_view name;
        int rs, rt, rd, shift, function;
    };
    const RegisterCase register_cases[] = {
        {0x012a4021, Operation::Addu, "addu", 9, 10, 8, 0, 0x21},
        {0x03e0f821, Operation::Addu, "addu", 31, 0, 31, 0, 0x21},
        {0x012a4023, Operation::Subu, "subu", 9, 10, 8, 0, 0x23},
        {0x012a4024, Operation::And, "and", 9, 10, 8, 0, 0x24},
        {0x012a4025, Operation::Or, "or", 9, 10, 8, 0, 0x25},
        {0x012a4026, Operation::Xor, "xor", 9, 10, 8, 0, 0x26},
        {0x03e00008, Operation::Jr, "jr", 31, 0, 0, 0, 8},
        {0x00000008, Operation::Jr, "jr", 0, 0, 0, 0, 8},
        {0x00094100, Operation::Sll, "sll", 0, 9, 8, 4, 0},
        {0x001fffc0, Operation::Sll, "sll", 0, 31, 31, 31, 0},
        {0x00000000, Operation::Sll, "sll", 0, 0, 0, 0, 0},
        {0x00094102, Operation::Srl, "srl", 0, 9, 8, 4, 2},
        {0x001fffc2, Operation::Srl, "srl", 0, 31, 31, 31, 2},
    };
    for (const auto& expected : register_cases) {
        const auto actual = decode(expected.word);
        check(actual.word == expected.word && actual.primary_opcode == 0,
              expected.word, "R-format word/opcode");
        check(actual.operation == expected.operation && mnemonic(actual.operation) == expected.name,
              expected.word, "operation/mnemonic");
        check(actual.rs == expected.rs && actual.rt == expected.rt && actual.rd == expected.rd
              && actual.shift_amount == expected.shift && actual.function == expected.function,
              expected.word, "R-format fields");
    }

    struct ImmediateCase {
        std::uint32_t word;
        Operation operation;
        std::string_view name;
        int opcode, rs, rt;
        std::uint16_t immediate;
        std::int32_t signed_value;
    };
    const ImmediateCase immediate_cases[] = {
        {0x25280001, Operation::Addiu, "addiu", 9, 9, 8, 0x0001, 1},
        {0x27bdfff0, Operation::Addiu, "addiu", 9, 29, 29, 0xfff0, -16},
        {0x2408ffff, Operation::Addiu, "addiu", 9, 0, 8, 0xffff, -1},
        {0x27ff8000, Operation::Addiu, "addiu", 9, 31, 31, 0x8000, -32768},
        {0x24087fff, Operation::Addiu, "addiu", 9, 0, 8, 0x7fff, 32767},
        {0x24080000, Operation::Addiu, "addiu", 9, 0, 8, 0x0000, 0},
        {0x3c081234, Operation::Lui, "lui", 15, 0, 8, 0x1234, 4660},
        {0x3c1fffff, Operation::Lui, "lui", 15, 0, 31, 0xffff, -1},
        {0x8fa80010, Operation::Lw, "lw", 35, 29, 8, 0x0010, 16},
        {0x8fe8fffc, Operation::Lw, "lw", 35, 31, 8, 0xfffc, -4},
        {0xafa80010, Operation::Sw, "sw", 43, 29, 8, 0x0010, 16},
        {0xafe8fffc, Operation::Sw, "sw", 43, 31, 8, 0xfffc, -4},
        {0x11090003, Operation::Beq, "beq", 4, 8, 9, 0x0003, 3},
        {0x1109fffe, Operation::Beq, "beq", 4, 8, 9, 0xfffe, -2},
        {0x15090003, Operation::Bne, "bne", 5, 8, 9, 0x0003, 3},
        {0x1509fffe, Operation::Bne, "bne", 5, 8, 9, 0xfffe, -2},
    };
    for (const auto& expected : immediate_cases) {
        const auto actual = decode(expected.word);
        check(actual.word == expected.word && actual.primary_opcode == expected.opcode,
              expected.word, "I-format word/opcode");
        check(actual.operation == expected.operation && mnemonic(actual.operation) == expected.name,
              expected.word, "operation/mnemonic");
        check(actual.rs == expected.rs && actual.rt == expected.rt
              && actual.immediate == expected.immediate
              && actual.signed_immediate() == expected.signed_value,
              expected.word, "I-format fields/sign extension");
    }

    struct JumpCase {
        std::uint32_t word;
        Operation operation;
        std::string_view name;
        int opcode;
        std::uint32_t index;
    };
    const JumpCase jump_cases[] = {
        {0x08040000, Operation::J, "j", 2, 0x00040000},
        {0x0bffffff, Operation::J, "j", 2, 0x03ffffff},
        {0x0c040000, Operation::Jal, "jal", 3, 0x00040000},
        {0x0c000000, Operation::Jal, "jal", 3, 0},
    };
    for (const auto& expected : jump_cases) {
        const auto actual = decode(expected.word);
        check(actual.word == expected.word && actual.primary_opcode == expected.opcode
              && actual.jump_index == expected.index, expected.word, "J-format fields");
        check(actual.operation == expected.operation && mnemonic(actual.operation) == expected.name,
              expected.word, "operation/mnemonic");
    }

    // Outside the implemented subset, plus nonzero fixed fields. Unsupported
    // is our policy; it makes no claim about a hardware reserved-instruction trap.
    const std::uint32_t unsupported[] = {
        0xffffffff, 0x70000000, 0x46000000, 0x34081234, 0x0000000c,
        0x00294100, 0x00294102, 0x03e10008, 0x03e00808, 0x03e00048,
        0x012a4061, 0x012a4063, 0x012a4064, 0x012a4065, 0x012a4066,
        0x3c281234,
    };
    for (const auto word : unsupported) {
        const auto actual = decode(word);
        check(actual.operation == Operation::Unsupported && actual.word == word
              && mnemonic(actual.operation) == "unsupported", word, "unsupported preserved");
    }

    const std::array<std::uint8_t, 4> stack_adjustment = {0xf0, 0xff, 0xbd, 0x27};
    check(read_instruction_word(stack_adjustment) == 0x27bdfff0, 0x27bdfff0, "little endian");
    const std::array<std::uint8_t, 4> high_bit = {0x10, 0x00, 0xa8, 0xaf};
    check(read_instruction_word(high_bit) == 0xafa80010, 0xafa80010, "unsigned byte shifts");

    if (failures != 0) {
        return 1;
    }
    std::cout << "33 hand-selected instructions, 16 unsupported encodings, endian checks passed\n";
    return 0;
}
