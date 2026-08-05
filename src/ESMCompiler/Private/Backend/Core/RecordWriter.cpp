#include "RecordWriter.h"

#include <limits>
#include <stdexcept>

RecordWriter::RecordWriter(BinaryWriter& writer, const RecordHeader& header)
    : writer_(writer) {
    writer_.WriteBytes(header.recordType.data(), header.recordType.size());

    dataSizePosition_ = writer_.Tell();
    writer_.WriteUInt32(0);
    writer_.WriteUInt32(header.flags);
    writer_.WriteUInt32(header.formID);
    writer_.WriteUInt32(header.revision);
    writer_.WriteUInt16(header.version);
    writer_.WriteUInt16(header.unknown);

    payloadStart_ = writer_.Tell();
}

RecordWriter::~RecordWriter() noexcept {
    if (!finalized_) {
        try {
            Finalize();
        } catch (...) {
            // A destructor cannot report an I/O failure while unwinding.
        }
    }
}

void RecordWriter::WriteUInt8(std::uint8_t value) {
    EnsurePayloadWritable();
    writer_.WriteUInt8(value);
}

void RecordWriter::WriteUInt16(std::uint16_t value) {
    EnsurePayloadWritable();
    writer_.WriteUInt16(value);
}

void RecordWriter::WriteUInt32(std::uint32_t value) {
    EnsurePayloadWritable();
    writer_.WriteUInt32(value);
}

void RecordWriter::WriteUInt64(std::uint64_t value) {
    EnsurePayloadWritable();
    writer_.WriteUInt64(value);
}

void RecordWriter::WriteInt8(std::int8_t value) {
    EnsurePayloadWritable();
    writer_.WriteInt8(value);
}

void RecordWriter::WriteInt16(std::int16_t value) {
    EnsurePayloadWritable();
    writer_.WriteInt16(value);
}

void RecordWriter::WriteInt32(std::int32_t value) {
    EnsurePayloadWritable();
    writer_.WriteInt32(value);
}

void RecordWriter::WriteInt64(std::int64_t value) {
    EnsurePayloadWritable();
    writer_.WriteInt64(value);
}

void RecordWriter::WriteFloat(float value) {
    EnsurePayloadWritable();
    writer_.WriteFloat(value);
}

void RecordWriter::WriteDouble(double value) {
    EnsurePayloadWritable();
    writer_.WriteDouble(value);
}

void RecordWriter::WriteBytes(const void* data, std::size_t size) {
    EnsurePayloadWritable();
    writer_.WriteBytes(data, size);
}

void RecordWriter::WriteString(std::string_view value) {
    EnsurePayloadWritable();
    writer_.WriteString(value);
}

void RecordWriter::Finalize() {
    if (finalized_) {
        return;
    }

    if (hasActiveSubrecord_) {
        throw std::logic_error("Cannot finalize a record while a subrecord is active.");
    }

    const std::streampos recordEnd = writer_.Tell();
    const std::streamoff payloadSize = recordEnd - payloadStart_;
    if (payloadSize < 0) {
        throw std::logic_error("RecordWriter payload end precedes its start.");
    }

    if (static_cast<std::uintmax_t>(payloadSize) > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("RecordWriter payload exceeds the 32-bit data-size field.");
    }

    writer_.Seek(dataSizePosition_);
    writer_.WriteUInt32(static_cast<std::uint32_t>(payloadSize));

    // Marking this first prevents the destructor from attempting a second patch
    // if restoring the write position itself reports an I/O error.
    finalized_ = true;
    writer_.Seek(recordEnd);
}

bool RecordWriter::IsFinalized() const noexcept {
    return finalized_;
}

void RecordWriter::EnsureOpen() const {
    if (finalized_) {
        throw std::logic_error("Cannot write to a finalized record.");
    }
}

void RecordWriter::EnsurePayloadWritable() const {
    EnsureOpen();
    if (hasActiveSubrecord_) {
        throw std::logic_error("Cannot write record payload while a subrecord is active.");
    }
}

void RecordWriter::OnSubrecordOpened() {
    EnsureOpen();
    if (hasActiveSubrecord_) {
        throw std::logic_error("Cannot create a nested or concurrent subrecord.");
    }

    hasActiveSubrecord_ = true;
}

void RecordWriter::OnSubrecordClosed() noexcept {
    hasActiveSubrecord_ = false;
}
