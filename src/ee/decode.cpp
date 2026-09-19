#include "gt4recomp/ee_decode.hpp"

namespace gt4recomp::ee {
namespace {

Operation decode_special(const DecodedInstruction& instruction) {
    switch (instruction.function) {
    case 0x00:
        return instruction.rs == 0 ? Operation::Sll : Operation::Unsupported;
    case 0x02:
        return instruction.rs == 0 ? Operation::Srl : Operation::Unsupported;
    case 0x08:
        if (instruction.rt == 0 && instruction.rd == 0 && instruction.shift_amount == 0) {
            return Operation::Jr;
        }
        return Operation::Unsupported;
    default:
        break;
    }

    // Register ALU encodings in this subset require a zero shift field.
    if (instruction.shift_amount != 0) {
        return Operation::Unsupported;
    }
    switch (instruction.function) {
    case 0x21: return Operation::Addu;
    case 0x23: return Operation::Subu;
    case 0x24: return Operation::And;
    case 0x25: return Operation::Or;
    case 0x26: return Operation::Xor;
    default: return Operation::Unsupported;
    }
}

} // namespace

std::int32_t DecodedInstruction::signed_immediate() const {
    const auto value = static_cast<std::int32_t>(immediate);
    return immediate >= 0x8000 ? value - 0x10000 : value;
}

std::uint32_t read_instruction_word(std::span<const std::uint8_t, 4> bytes) {
    // Promote before shifting: the top byte must remain unsigned.
    return static_cast<std::uint32_t>(bytes[0])
        | (static_cast<std::uint32_t>(bytes[1]) << 8)
        | (static_cast<std::uint32_t>(bytes[2]) << 16)
        | (static_cast<std::uint32_t>(bytes[3]) << 24);
}

DecodedInstruction decode(std::uint32_t word) {
    DecodedInstruction result;
    result.word = word;
    result.primary_opcode = static_cast<std::uint8_t>((word >> 26) & 0x3f);
    result.rs = static_cast<std::uint8_t>((word >> 21) & 0x1f);
    result.rt = static_cast<std::uint8_t>((word >> 16) & 0x1f);
    result.rd = static_cast<std::uint8_t>((word >> 11) & 0x1f);
    result.shift_amount = static_cast<std::uint8_t>((word >> 6) & 0x1f);
    result.function = static_cast<std::uint8_t>(word & 0x3f);
    result.immediate = static_cast<std::uint16_t>(word & 0xffff);
    result.jump_index = word & 0x03ffffff;

    switch (result.primary_opcode) {
    case 0x00: result.operation = decode_special(result); break;
    case 0x02: result.operation = Operation::J; break;
    case 0x03: result.operation = Operation::Jal; break;
    case 0x04: result.operation = Operation::Beq; break;
    case 0x05: result.operation = Operation::Bne; break;
    case 0x09: result.operation = Operation::Addiu; break;
    case 0x0f:
        if (result.rs == 0) {
            result.operation = Operation::Lui;
        }
        break;
    case 0x23: result.operation = Operation::Lw; break;
    case 0x2b: result.operation = Operation::Sw; break;
    default: break; // Preserve the original word and fields for diagnostics.
    }
    return result;
}

std::string_view mnemonic(Operation operation) {
    switch (operation) {
    case Operation::Addiu: return "addiu";
    case Operation::Addu: return "addu";
    case Operation::Subu: return "subu";
    case Operation::And: return "and";
    case Operation::Or: return "or";
    case Operation::Xor: return "xor";
    case Operation::Lui: return "lui";
    case Operation::Lw: return "lw";
    case Operation::Sw: return "sw";
    case Operation::Beq: return "beq";
    case Operation::Bne: return "bne";
    case Operation::J: return "j";
    case Operation::Jal: return "jal";
    case Operation::Jr: return "jr";
    case Operation::Sll: return "sll";
    case Operation::Srl: return "srl";
    case Operation::Unsupported: return "unsupported";
    }
    return "unsupported";
}

} // namespace gt4recomp::ee
