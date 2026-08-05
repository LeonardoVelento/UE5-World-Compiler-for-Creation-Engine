#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <string_view>

class BinaryWriter final {
public:
    explicit BinaryWriter(const std::filesystem::path& path);
    ~BinaryWriter() noexcept = default;

    BinaryWriter(const BinaryWriter&) = delete;
    BinaryWriter& operator=(const BinaryWriter&) = delete;
    BinaryWriter(BinaryWriter&&) = default;
    BinaryWriter& operator=(BinaryWriter&&) = default;

    void WriteUInt8(std::uint8_t value);
    void WriteUInt16(std::uint16_t value);
    void WriteUInt32(std::uint32_t value);
    void WriteUInt64(std::uint64_t value);

    void WriteInt8(std::int8_t value);
    void WriteInt16(std::int16_t value);
    void WriteInt32(std::int32_t value);
    void WriteInt64(std::int64_t value);

    void WriteFloat(float value);
    void WriteDouble(double value);

    void WriteBytes(const void* data, std::size_t size);

    // Writes the string bytes exactly as supplied: no length prefix or terminator.
    void WriteString(std::string_view value);

    [[nodiscard]] std::streampos Tell();
    void Seek(std::streampos position);
    void Seek(std::streamoff offset, std::ios_base::seekdir direction);

private:
    std::ofstream stream_;
};
