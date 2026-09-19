#include "gt4recomp/executable_image.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <zlib.h>

using Bytes = std::vector<std::uint8_t>;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void append_u32(Bytes& bytes, std::uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) bytes.push_back(static_cast<std::uint8_t>(value >> shift));
}

void replace_u32(Bytes& bytes, std::size_t offset, std::uint32_t value) {
    for (unsigned index = 0; index < 4; ++index) bytes.at(offset + index) = static_cast<std::uint8_t>(value >> (index * 8));
}

std::uint32_t read_u32(const Bytes& bytes, std::size_t offset) {
    return bytes.at(offset) | (std::uint32_t{bytes.at(offset + 1)} << 8)
        | (std::uint32_t{bytes.at(offset + 2)} << 16) | (std::uint32_t{bytes.at(offset + 3)} << 24);
}

Bytes body_fixture() {
    Bytes body{0, 0, 0, 0}; // Two empty authentication values in this synthetic format fixture.
    append_u32(body, 3);
    append_u32(body, 0x100008);
    append_u32(body, 0x100008); append_u32(body, 24);
    body.insert(body.end(), 24, 0xaa);
    append_u32(body, 0x100000); append_u32(body, 32);
    for (std::uint8_t byte = 0; byte < 32; ++byte) body.push_back(byte);
    append_u32(body, 0x100080); append_u32(body, 8);
    body.insert(body.end(), 8, 0xbb);
    return body;
}

Bytes compress_core(const Bytes& body) {
    Bytes compressed(compressBound(static_cast<uLong>(body.size())));
    z_stream stream{};
    require(deflateInit2(&stream, 6, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY) == Z_OK, "deflate init");
    stream.next_in = const_cast<Bytef*>(body.data()); stream.avail_in = static_cast<uInt>(body.size());
    stream.next_out = compressed.data(); stream.avail_out = static_cast<uInt>(compressed.size());
    const int result = deflate(&stream, Z_FINISH);
    const auto size = stream.total_out;
    deflateEnd(&stream);
    require(result == Z_STREAM_END, "deflate fixture");
    compressed.resize(size);
    Bytes core{1, 1};
    append_u32(core, static_cast<std::uint32_t>(body.size()));
    core.insert(core.end(), compressed.begin(), compressed.end());
    return core;
}

void rejects(const std::function<void()>& action, const std::string& diagnostic) {
    try { action(); }
    catch (const std::runtime_error& error) {
        require(std::string(error.what()).find(diagnostic) != std::string::npos, "Wrong rejection reason");
        return;
    }
    throw std::runtime_error("Expected rejection: " + diagnostic);
}

int main() {
    try {
        const auto body = body_fixture();
        const auto core = compress_core(body);
        const auto image = gt4recomp::reconstruct_core(core);
        require(image.entry_address == 0x100008, "entry not preserved");
        require(image.reginfo.guest_address == 0x100008, "raw reginfo address changed");
        require(image.text.bytes.size() == 32 && image.text.bytes[31] == 31, "text bytes changed");
        require(image.data.bytes == Bytes(8, 0xbb), "data bytes changed");

        const auto elf = gt4recomp::make_reference_analysis_elf(image);
        require(elf == gt4recomp::make_reference_analysis_elf(image), "ELF nondeterministic");
        require(elf[0] == 0x7f && elf[4] == 1 && elf[5] == 1, "ELF identity");
        require(read_u32(elf, 24) == image.entry_address, "ELF entry");
        require(read_u32(elf, 32) == 0, "unexpected section table");
        for (unsigned index = 0; index < 3; ++index) {
            const std::size_t header = 52 + index * 32;
            const auto offset = read_u32(elf, header + 4);
            const auto address = read_u32(elf, header + 8);
            const auto alignment = read_u32(elf, header + 28);
            require(offset % alignment == address % alignment, "ELF alignment congruence");
        }
        require(read_u32(elf, 84 + 8) == 0x100020, "analysis reginfo policy");
        require(read_u32(elf, 116 + 20) == 8 + 0x800000, "zero-fill policy");
        const auto data_offset = read_u32(elf, 116 + 4);
        require(elf.size() == data_offset + 8, "BSS was stored in the ELF file");
        require(std::equal(image.data.bytes.begin(), image.data.bytes.end(), elf.begin() + data_offset), "ELF payload");

        rejects([] { (void)gt4recomp::reconstruct_core({}); }, "Truncated");
        auto broken = core; broken[0] = 2;
        rejects([&] { (void)gt4recomp::reconstruct_core(broken); }, "flags");
        broken = core; replace_u32(broken, 2, 0xffffffff);
        rejects([&] { (void)gt4recomp::reconstruct_core(broken); }, "size");
        broken = core; broken.pop_back();
        rejects([&] { (void)gt4recomp::reconstruct_core(broken); }, "DEFLATE");
        broken = core; broken.push_back(0);
        rejects([&] { (void)gt4recomp::reconstruct_core(broken); }, "DEFLATE");
        broken = core; replace_u32(broken, 2, static_cast<std::uint32_t>(body.size() - 1));
        rejects([&] { (void)gt4recomp::reconstruct_core(broken); }, "size mismatch");

        auto bad_body = body; replace_u32(bad_body, 4, 4);
        rejects([&] { (void)gt4recomp::reconstruct_core(compress_core(bad_body)); }, "record count");
        bad_body = body; bad_body[0] = 0xff; bad_body[1] = 0xff;
        rejects([&] { (void)gt4recomp::reconstruct_core(compress_core(bad_body)); }, "Truncated");
        bad_body = body; replace_u32(bad_body, 16, 0xffffffff);
        rejects([&] { (void)gt4recomp::reconstruct_core(compress_core(bad_body)); }, "Truncated");
        bad_body = body; bad_body.push_back(0);
        rejects([&] { (void)gt4recomp::reconstruct_core(compress_core(bad_body)); }, "Unexplained");
        bad_body = body; replace_u32(bad_body, 8, 0x100001);
        rejects([&] { (void)gt4recomp::reconstruct_core(compress_core(bad_body)); }, "aligned");
        bad_body = body; replace_u32(bad_body, 8, 0x200000);
        rejects([&] { (void)gt4recomp::reconstruct_core(compress_core(bad_body)); }, "outside");
        bad_body = body; replace_u32(bad_body, 84, 0xfffffff0);
        rejects([&] { (void)gt4recomp::reconstruct_core(compress_core(bad_body)); }, "RAM range");
        auto invalid_image = image; invalid_image.data.guest_address = 0x100004;
        rejects([&] { (void)gt4recomp::make_reference_analysis_elf(invalid_image); }, "overlap");
        invalid_image = image; invalid_image.data.guest_address = 0x1fffff0;
        rejects([&] { (void)gt4recomp::make_reference_analysis_elf(invalid_image); }, "zero-fill");
        invalid_image = image; invalid_image.reginfo.guest_address = 0x100000;
        rejects([&] { (void)gt4recomp::make_reference_analysis_elf(invalid_image); }, "reginfo view");
        std::cout << "Image reconstruction, ELF layout and 16 rejection cases passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
