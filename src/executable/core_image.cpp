#include "gt4recomp/executable_image.hpp"

#include <stdexcept>
#include <string>
#include <zlib.h>

namespace gt4recomp {
namespace {

constexpr std::uint32_t ee_memory_size = 32 * 1024 * 1024;
constexpr std::uint32_t maximum_core_size = 32 * 1024 * 1024;

class ByteReader {
public:
    explicit ByteReader(std::span<const std::uint8_t> bytes) : bytes_(bytes) {}

    std::uint16_t read_u16() {
        const auto bytes = take(2);
        return static_cast<std::uint16_t>(bytes[0] | (std::uint16_t{bytes[1]} << 8));
    }

    std::uint32_t read_u32() {
        const auto bytes = take(4);
        std::uint32_t value = 0;
        for (unsigned index = 0; index < 4; ++index) {
            value |= std::uint32_t{bytes[index]} << (index * 8);
        }
        return value;
    }

    std::span<const std::uint8_t> take(std::size_t count) {
        if (count > bytes_.size() - position_) {
            throw std::runtime_error("Truncated CORE field at byte " + std::to_string(position_));
        }
        const auto result = bytes_.subspan(position_, count);
        position_ += count;
        return result;
    }

    bool at_end() const { return position_ == bytes_.size(); }

private:
    std::span<const std::uint8_t> bytes_;
    std::size_t position_ = 0;
};

std::vector<std::uint8_t> inflate_body(std::span<const std::uint8_t> core) {
    if (core.size() > maximum_core_size) {
        throw std::runtime_error("CORE exceeds the supported size limit");
    }
    ByteReader header(core);
    if (header.read_u16() != 0x0101) {
        throw std::runtime_error("Unsupported CORE flags; expected unencrypted 0x0101");
    }
    const auto expected_size = header.read_u32();
    if (expected_size == 0 || expected_size > maximum_core_size) {
        throw std::runtime_error("Invalid or excessive inflated CORE size");
    }

    // An extra byte lets zlib expose output larger than the declared size.
    std::vector<std::uint8_t> output(expected_size + 1);
    const auto compressed = core.subspan(6);
    z_stream stream{};
    // zlib's API predates const-correctness; inflate does not modify input bytes.
    stream.next_in = const_cast<Bytef*>(compressed.data());
    stream.avail_in = static_cast<uInt>(compressed.size());
    stream.next_out = output.data();
    stream.avail_out = static_cast<uInt>(output.size());
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
        throw std::runtime_error("Could not initialize raw DEFLATE decompression");
    }
    const int result = inflate(&stream, Z_FINISH);
    const auto consumed_size = stream.total_in;
    const auto produced_size = stream.total_out;
    inflateEnd(&stream);
    if (result != Z_STREAM_END || consumed_size != compressed.size() || produced_size != expected_size) {
        throw std::runtime_error("Invalid DEFLATE stream, trailing bytes, or inflated size mismatch");
    }
    output.resize(expected_size);
    return output;
}

ImageRecord read_record(ByteReader& reader) {
    ImageRecord record;
    record.guest_address = reader.read_u32();
    const auto size = reader.read_u32();
    const auto payload = reader.take(size);
    record.bytes.assign(payload.begin(), payload.end());
    return record;
}

std::uint64_t record_end(const ImageRecord& record) {
    return std::uint64_t{record.guest_address} + record.bytes.size();
}

void validate_record(const ImageRecord& record, const char* name) {
    if (record.bytes.empty() || record_end(record) > ee_memory_size) {
        throw std::runtime_error(std::string(name) + " payload lies outside the supported EE RAM range");
    }
}

} // namespace

void validate_image(const ExecutableImage& image) {
    validate_record(image.reginfo, "reginfo");
    validate_record(image.text, "text");
    validate_record(image.data, "data");
    if (image.reginfo.bytes.size() != 24) {
        throw std::runtime_error("Expected a 24-byte reginfo record");
    }
    if (image.text.guest_address % 4 != 0 || image.entry_address % 4 != 0) {
        throw std::runtime_error("Text and entry addresses must be instruction-aligned");
    }
    if (image.entry_address < image.text.guest_address ||
        std::uint64_t{image.entry_address} + 4 > record_end(image.text)) {
        throw std::runtime_error("Entry instruction is outside the text payload");
    }
    if (record_end(image.text) > image.data.guest_address) {
        throw std::runtime_error("Text/data records overlap or are out of order");
    }
    // Reginfo is supplementary metadata, not an independently copied RAM region.
    // Its overlap with text in this revision is retained rather than overwritten.
}

ExecutableImage reconstruct_core(std::span<const std::uint8_t> core) {
    const auto inflated = inflate_body(core);
    ByteReader reader(inflated);
    for (unsigned index = 0; index < 2; ++index) {
        const auto authentication_size = reader.read_u16();
        reader.take(authentication_size);
    }
    if (reader.read_u32() != 3) {
        throw std::runtime_error("Unsupported CORE record count; expected three");
    }
    ExecutableImage image;
    image.entry_address = reader.read_u32();
    image.reginfo = read_record(reader);
    image.text = read_record(reader);
    image.data = read_record(reader);
    if (!reader.at_end()) {
        throw std::runtime_error("Unexplained bytes after the last CORE record");
    }
    validate_image(image);
    return image;
}

} // namespace gt4recomp
