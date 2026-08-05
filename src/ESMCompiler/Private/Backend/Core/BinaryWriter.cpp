#include "BinaryWriter.h"

#include <array>
#include <bit>
#include <climits>
#include <limits>
#include <stdexcept>
#include <type_traits>

namespace {

static_assert(CHAR_BIT == 8, "BinaryWriter requires 8-bit bytes.");
static_assert(std::numeric_limits<float>::is_iec559 && sizeof(float) == sizeof(std::uint32_t),
              "BinaryWriter requires 32-bit IEEE 754 float values.");
static_assert(std::numeric_limits<double>::is_iec559 && sizeof(double) == sizeof(std::uint64_t),
              "BinaryWriter requires 64-bit IEEE 754 double values.");

template <typename UInt>
void WriteLittleEndian(std::ofstream& stream, UInt value) {
    static_assert(std::is_unsigned_v<UInt>);

    std::array<std::uint8_t, sizeof(UInt)> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = static_cast<std::uint8_t>(value & 0xFFu);
        value >>= CHAR_BIT;
    }

    stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
}

} // namespace

BinaryWriter::BinaryWriter(const std::filesystem::path& path) {
    stream_.exceptions(std::ios::badbit | std::ios::failbit);
    stream_.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
}

void BinaryWriter::WriteUInt8(std::uint8_t value) {
    WriteLittleEndian(stream_, value);
}

void BinaryWriter::WriteUInt16(std::uint16_t value) {
    WriteLittleEndian(stream_, value);
}

void BinaryWriter::WriteUInt32(std::uint32_t value) {
    WriteLittleEndian(stream_, value);
}

void BinaryWriter::WriteUInt64(std::uint64_t value) {
    WriteLittleEndian(stream_, value);
}

void BinaryWriter::WriteInt8(std::int8_t value) {
    WriteUInt8(static_cast<std::uint8_t>(value));
}

void BinaryWriter::WriteInt16(std::int16_t value) {
    WriteUInt16(static_cast<std::uint16_t>(value));
}

void BinaryWriter::WriteInt32(std::int32_t value) {
    WriteUInt32(static_cast<std::uint32_t>(value));
}

void BinaryWriter::WriteInt64(std::int64_t value) {
    WriteUInt64(static_cast<std::uint64_t>(value));
}

void BinaryWriter::WriteFloat(float value) {
    WriteUInt32(std::bit_cast<std::uint32_t>(value));
}

void BinaryWriter::WriteDouble(double value) {
    WriteUInt64(std::bit_cast<std::uint64_t>(value));
}

void BinaryWriter::WriteBytes(const void* data, std::size_t size) {
    if (size == 0) {
        return;
    }

    if (data == nullptr) {
        throw std::invalid_argument("BinaryWriter::WriteBytes received a null data pointer.");
    }

    const auto maxStreamSize = static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max());
    if (size > maxStreamSize) {
        throw std::length_error("BinaryWriter::WriteBytes size exceeds std::streamsize capacity.");
    }

    stream_.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
}

void BinaryWriter::WriteString(std::string_view value) {
    WriteBytes(value.data(), value.size());
}

std::streampos BinaryWriter::Tell() {
    return stream_.tellp();
}

void BinaryWriter::Seek(std::streampos position) {
    stream_.seekp(position);
}

void BinaryWriter::Seek(std::streamoff offset, std::ios_base::seekdir direction) {
    stream_.seekp(offset, direction);
}
