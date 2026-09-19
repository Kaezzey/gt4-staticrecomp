#include "gt4recomp/executable_image.hpp"

#include <array>
#include <stdexcept>

namespace gt4recomp {
namespace {

constexpr std::uint32_t elf_header_size = 52;
constexpr std::uint32_t program_header_size = 32;
constexpr std::uint32_t reference_zero_fill_size = 8 * 1024 * 1024;
constexpr std::uint32_t reference_mips_flags = 0x20924001;
constexpr std::uint32_t ee_memory_size = 32 * 1024 * 1024;

struct OutputSegment {
    std::uint32_t type;
    std::uint32_t file_offset;
    std::uint32_t guest_address;
    std::uint32_t memory_size;
    std::uint32_t flags;
    std::uint32_t alignment;
    const std::vector<std::uint8_t>* payload;
};

class ElfWriter {
public:
    void write_u16(std::uint16_t value) {
        bytes.push_back(static_cast<std::uint8_t>(value));
        bytes.push_back(static_cast<std::uint8_t>(value >> 8));
    }

    void write_u32(std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            bytes.push_back(static_cast<std::uint8_t>(value >> shift));
        }
    }

    std::vector<std::uint8_t> bytes;
};

// Find the next file position congruent to the guest address modulo alignment.
std::uint32_t place_segment(std::uint32_t cursor, std::uint32_t address, std::uint32_t alignment) {
    const auto target_remainder = address % alignment;
    const auto current_remainder = cursor % alignment;
    const auto padding = (target_remainder + alignment - current_remainder) % alignment;
    return cursor + padding;
}

void write_header(ElfWriter& output, std::uint32_t entry) {
    output.bytes = {0x7f, 'E', 'L', 'F', 1, 1, 1}; // ELF32, little endian, version 1.
    output.bytes.resize(16, 0);
    output.write_u16(2); // ET_EXEC
    output.write_u16(8); // EM_MIPS
    output.write_u32(1); // ELF version
    output.write_u32(entry);
    output.write_u32(elf_header_size); // Program header table follows ELF header.
    output.write_u32(0); // No section table: program headers describe this analysis image.
    output.write_u32(reference_mips_flags);
    output.write_u16(elf_header_size);
    output.write_u16(program_header_size);
    output.write_u16(3); // Two PT_LOAD entries and supplementary PT_MIPS_REGINFO.
    output.write_u16(0); // Section header size
    output.write_u16(0); // Section count
    output.write_u16(0); // Section-name table index
}

void write_program_header(ElfWriter& output, const OutputSegment& segment) {
    output.write_u32(segment.type);
    output.write_u32(segment.file_offset);
    output.write_u32(segment.guest_address);
    output.write_u32(segment.guest_address); // Physical address matches the reference view.
    output.write_u32(static_cast<std::uint32_t>(segment.payload->size()));
    output.write_u32(segment.memory_size);
    output.write_u32(segment.flags);
    output.write_u32(segment.alignment);
}

} // namespace

std::vector<std::uint8_t> make_reference_analysis_elf(const ExecutableImage& image) {
    validate_image(image);
    const auto text_size = static_cast<std::uint32_t>(image.text.bytes.size());
    const auto data_size = static_cast<std::uint32_t>(image.data.bytes.size());
    const auto reginfo_address = image.reginfo.guest_address + 24;
    const auto data_memory_size = data_size + reference_zero_fill_size;
    if (std::uint64_t{image.data.guest_address} + data_memory_size > ee_memory_size) {
        throw std::runtime_error("Reference zero-fill policy would exceed EE RAM");
    }
    if (reginfo_address < std::uint64_t{image.text.guest_address} + text_size ||
        std::uint64_t{reginfo_address} + 24 > image.data.guest_address) {
        throw std::runtime_error("Reference reginfo view does not fit between text and data");
    }

    const auto text_offset = place_segment(elf_header_size + 3 * program_header_size,
                                          image.text.guest_address, 0x1000);
    const auto reginfo_offset = place_segment(text_offset + text_size, reginfo_address, 4);
    const auto data_offset = place_segment(reginfo_offset + 24, image.data.guest_address, 0x1000);
    const std::array<OutputSegment, 3> segments{{
        {1, text_offset, image.text.guest_address, text_size, 7, 0x1000, &image.text.bytes},
        {0x70000000, reginfo_offset, reginfo_address, 24, 4, 4, &image.reginfo.bytes},
        {1, data_offset, image.data.guest_address, data_memory_size, 6, 0x1000, &image.data.bytes},
    }};

    ElfWriter output;
    write_header(output, image.entry_address);
    for (const auto& segment : segments) {
        write_program_header(output, segment);
    }
    for (const auto& segment : segments) {
        output.bytes.resize(segment.file_offset, 0);
        output.bytes.insert(output.bytes.end(), segment.payload->begin(), segment.payload->end());
    }
    return output.bytes;
}

} // namespace gt4recomp
