#include "gt4recomp/ee_disassemble.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>

int main() {
    using gt4recomp::ee::format_instruction;
    using gt4recomp::ee::disassemble_region;
    int failures = 0;
    const auto check = [&](bool passed, const char* label) {
        if (!passed) { std::cerr << label << '\n'; ++failures; }
    };
    struct Case { std::uint32_t word, pc; const char* expected; };
    const Case cases[] = {
        {0x3044000f, 0, "andi a0, v0, 0xf"},
        {0x33ffffff, 0, "andi ra, ra, 0xffff"},
        {0x3484fff0, 0, "ori a0, a0, 0xfff0"},
        {0x34028000, 0, "ori v0, zero, 0x8000"},
        {0x27bdfff0, 0, "addiu sp, sp, -0x10"},
        {0x24088000, 0, "addiu t0, zero, -0x8000"},
        {0x012a4021, 0, "addu t0, t1, t2"},
        {0x012a4023, 0, "subu t0, t1, t2"},
        {0x012a4024, 0, "and t0, t1, t2"},
        {0x012a4025, 0, "or t0, t1, t2"},
        {0x012a4026, 0, "xor t0, t1, t2"},
        {0x3c1fffff, 0, "lui ra, 0xffff"},
        {0x8fa80010, 0, "lw t0, 0x10(sp)"},
        {0xafa8fffc, 0, "sw t0, -0x4(sp)"},
        {0x11090003, 0x100090, "beq t0, t1, 0x001000a0"},
        {0x1509fffe, 0x1000a0, "bne t0, t1, 0x0010009c"},
        {0x1109fffe, 0, "beq t0, t1, 0xfffffffc"},
        {0x11090000, 0xfffffffc, "beq t0, t1, 0x00000000"},
        {0x08040000, 0x0ffffffc, "j 0x10100000"},
        {0x0c040000, 0xfffffffc, "jal 0x00100000"},
        {0x03e00008, 0, "jr ra"},
        {0x00094100, 0, "sll t0, t1, 0x4"},
        {0x001fffc2, 0, "srl ra, ra, 0x1f"},
        {0, 0, "sll zero, zero, 0x0"},
        {0x70000000, 0, "unsupported 0x70000000 ; opcode=0x1c function=0x00"},
    };
    for (const auto& item : cases) {
        check(format_instruction(item.word, item.pc) == item.expected, item.expected);
    }
    gt4recomp::ImageRecord text{0x100000, {0xf0, 0xff, 0xbd, 0x27, 0, 0, 0, 0x70}};
    std::ostringstream listing, report;
    disassemble_region(text, 0x100000, 2, listing, report);
    check(listing.str() == "00100000: 27bdfff0  addiu sp, sp, -0x10\n"
                          "00100004: 70000000  unsupported 0x70000000 ; opcode=0x1c function=0x00\n",
          "complete sequential listing");
    check(report.str() == "region=0x00100000 words=2 supported=1 unsupported=1\n"
                         "unsupported opcode=0x1c function=0x00 count=1\n", "opcode report");
    for (const auto& [start, count] : {
        std::pair{0x100000u, 0u}, {0x100001u, 1u}, {0xffffcu, 1u},
        {0x100004u, 2u}, {0x100008u, 1u}, {0x100000u, 0xffffffffu}}) {
        std::ostringstream invalid_listing, invalid_report;
        bool rejected = false;
        try { disassemble_region(text, start, count, invalid_listing, invalid_report); }
        catch (const std::runtime_error&) { rejected = true; }
        check(rejected && invalid_listing.str().empty() && invalid_report.str().empty(),
              "bad range rejected before output");
    }
    gt4recomp::ImageRecord top{0xfffffffcu, {0, 0, 0, 0}};
    std::ostringstream top_listing, top_report;
    disassemble_region(top, 0xfffffffcu, 1, top_listing, top_report);
    check(top_listing.str().starts_with("fffffffc:"), "last address does not wrap traversal");
    std::ostringstream failed_listing, unused_report;
    failed_listing.setstate(std::ios::badbit);
    bool write_rejected = false;
    try { disassemble_region(text, 0x100000, 1, failed_listing, unused_report); }
    catch (const std::runtime_error&) { write_rejected = true; }
    check(write_rejected, "failed output stream rejected");
    return failures == 0 ? 0 : 1;
}
