#include "SubRecordWriter.h"

#include <limits>
#include <stdexcept>

SubRecordWriter::SubRecordWriter(RecordWriter& record, std::array<char, 4> subrecordType)
    : record_(record), writer_(record.writer_) {
    record_.OnSubrecordOpened();

    try {
        writer_.WriteBytes(subrecordType.data(), subrecordType.size());
        payloadSizePosition_ = writer_.Tell();
        writer_.WriteUInt16(0);
        payloadStart_ = writer_.Tell();
    } catch (...) {
        record_.OnSubrecordClosed();
        throw;
    }
}

SubRecordWriter::~SubRecordWriter() noexcept {
    if (!finalized_) {
        try {
            Finalize();
        } catch (...) {
            // A destructor cannot report an I/O failure while unwinding.
        }
    }
}

void SubRecordWriter::WriteUInt8(std::uint8_t value) {
    EnsureOpen();
    writer_.WriteUInt8(value);
}

void SubRecordWriter::WriteUInt16(std::uint16_t value) {
    EnsureOpen();
    writer_.WriteUInt16(value);
}

void SubRecordWriter::WriteUInt32(std::uint32_t value) {
    EnsureOpen();
    writer_.WriteUInt32(value);
}

void SubRecordWriter::WriteUInt64(std::uint64_t value) {
    EnsureOpen();
    writer_.WriteUInt64(value);
}

void SubRecordWriter::WriteInt8(std::int8_t value) {
    EnsureOpen();
    writer_.WriteInt8(value);
}

void SubRecordWriter::WriteInt16(std::int16_t value) {
    EnsureOpen();
    writer_.WriteInt16(value);
}

void SubRecordWriter::WriteInt32(std::int32_t value) {
    EnsureOpen();
    writer_.WriteInt32(value);
}

void SubRecordWriter::WriteInt64(std::int64_t value) {
    EnsureOpen();
    writer_.WriteInt64(value);
}

void SubRecordWriter::WriteFloat(float value) {
    EnsureOpen();
    writer_.WriteFloat(value);
}

void SubRecordWriter::WriteDouble(double value) {
    EnsureOpen();
    writer_.WriteDouble(value);
}

void SubRecordWriter::WriteBytes(const void* data, std::size_t size) {
    EnsureOpen();
    writer_.WriteBytes(data, size);
}

void SubRecordWriter::WriteString(std::string_view value) {
    EnsureOpen();
    writer_.WriteString(value);
}

void SubRecordWriter::Finalize() {
    if (finalized_) {
        return;
    }

    const std::streampos subrecordEnd = writer_.Tell();
    const std::streamoff payloadSize = subrecordEnd - payloadStart_;
    if (payloadSize < 0) {
        throw std::logic_error("SubRecordWriter payload end precedes its start.");
    }

    if (static_cast<std::uintmax_t>(payloadSize) > std::numeric_limits<std::uint16_t>::max()) {
        throw std::length_error("SubRecordWriter payload exceeds the 16-bit size field; XXXX is not supported.");
    }

    writer_.Seek(payloadSizePosition_);
    writer_.WriteUInt16(static_cast<std::uint16_t>(payloadSize));

    // Mark this subrecord complete before restoring the position so a fatal I/O
    // failure cannot cause a second attempt to patch the same size field.
    finalized_ = true;
    record_.OnSubrecordClosed();
    writer_.Seek(subrecordEnd);
}

bool SubRecordWriter::IsFinalized() const noexcept {
    return finalized_;
}

void SubRecordWriter::EnsureOpen() const {
    if (finalized_) {
        throw std::logic_error("Cannot write to a finalized subrecord.");
    }

    record_.EnsureOpen();
}
