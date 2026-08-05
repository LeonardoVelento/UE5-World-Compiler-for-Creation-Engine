#pragma once

#include "BinaryWriter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

class SubRecordWriter;

struct RecordHeader {
    std::array<char, 4> recordType{};
    std::uint32_t flags = 0;
    std::uint32_t formID = 0;
    std::uint32_t revision = 0;
    std::uint16_t version = 0;
    std::uint16_t unknown = 0;
};

// Owns an in-progress record. The payload size is backpatched on finalization.
class [[nodiscard("A RecordWriter must remain alive while writing its payload.")]] RecordWriter final {
public:
    RecordWriter(BinaryWriter& writer, const RecordHeader& header);
    ~RecordWriter() noexcept;

    RecordWriter(const RecordWriter&) = delete;
    RecordWriter& operator=(const RecordWriter&) = delete;
    RecordWriter(RecordWriter&&) = delete;
    RecordWriter& operator=(RecordWriter&&) = delete;

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

    // Patches the record payload size. Call this explicitly to observe I/O errors.
    void Finalize();
    [[nodiscard]] bool IsFinalized() const noexcept;

private:
    friend class SubRecordWriter;

    void EnsureOpen() const;
    void EnsurePayloadWritable() const;
    void OnSubrecordOpened();
    void OnSubrecordClosed() noexcept;

    BinaryWriter& writer_;
    std::streampos dataSizePosition_;
    std::streampos payloadStart_;
    bool hasActiveSubrecord_ = false;
    bool finalized_ = false;
};
