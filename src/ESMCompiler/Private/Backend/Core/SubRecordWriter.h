#pragma once

#include "RecordWriter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

// Owns an in-progress subrecord within a RecordWriter.
class [[nodiscard("A SubRecordWriter must remain alive while writing its payload.")]] SubRecordWriter final {
public:
    SubRecordWriter(RecordWriter& record, std::array<char, 4> subrecordType);
    ~SubRecordWriter() noexcept;

    SubRecordWriter(const SubRecordWriter&) = delete;
    SubRecordWriter& operator=(const SubRecordWriter&) = delete;
    SubRecordWriter(SubRecordWriter&&) = delete;
    SubRecordWriter& operator=(SubRecordWriter&&) = delete;

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
    void WriteString(std::string_view value);

    // Patches the subrecord payload size. Payloads larger than uint16 are rejected;
    // this writer does not implement the Creation Engine XXXX extension.
    void Finalize();
    [[nodiscard]] bool IsFinalized() const noexcept;

private:
    void EnsureOpen() const;

    RecordWriter& record_;
    BinaryWriter& writer_;
    std::streampos payloadSizePosition_;
    std::streampos payloadStart_;
    bool finalized_ = false;
};
