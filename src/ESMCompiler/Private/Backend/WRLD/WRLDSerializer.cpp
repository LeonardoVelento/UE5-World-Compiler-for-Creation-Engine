#include "WRLDSerializer.h"

#include "RecordWriter.h"
#include "SubRecordWriter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

void ValidateCString(const std::string& value, const char* fieldName) {
    if (value.find('\0') != std::string::npos) {
        throw std::invalid_argument(std::string("WRLD ") + fieldName +
                                    " cannot contain a null character.");
    }

    constexpr std::size_t kTerminatorSize = 1;
    constexpr std::size_t kMaxTextSize =
        static_cast<std::size_t>(std::numeric_limits<std::uint16_t>::max()) - kTerminatorSize;
    if (value.size() > kMaxTextSize) {
        throw std::length_error(std::string("WRLD ") + fieldName +
                                " exceeds the subrecord size limit.");
    }
}

void WriteCStringSubrecord(RecordWriter& record,
                           std::array<char, 4> type,
                           const std::string& value) {
    SubRecordWriter subrecord(record, type);
    subrecord.WriteString(value);
    subrecord.WriteUInt8(0);
    subrecord.Finalize();
}

void WriteFormIDSubrecord(RecordWriter& record,
                          std::array<char, 4> type,
                          RecordFormID formID) {
    SubRecordWriter subrecord(record, type);
    subrecord.WriteUInt32(formID);
    subrecord.Finalize();
}

} // namespace

void WRLDSerializer::Serialize(const WRLDRecord& record,
                               const RecordHeader& header,
                               BinaryWriter& writer) const {
    ValidateCString(record.editorID, "EditorID");
    ValidateCString(record.fullName, "FullName");

    const auto worldFlags = static_cast<std::uint8_t>(record.worldFlags);

    constexpr std::array<char, 4> kWRLDType = {'W', 'R', 'L', 'D'};
    if (header.recordType != kWRLDType) {
        throw std::invalid_argument("WRLDSerializer requires a WRLD record header.");
    }

    if (header.formID == 0) {
        throw std::invalid_argument("WRLDSerializer requires a non-zero WRLD FormID.");
    }

    RecordWriter wrld(writer, header);

    WriteCStringSubrecord(wrld, {'E', 'D', 'I', 'D'}, record.editorID);
    WriteCStringSubrecord(wrld, {'F', 'U', 'L', 'L'}, record.fullName);

    if (record.parentWorld.has_value()) {
        WriteFormIDSubrecord(wrld, {'W', 'N', 'A', 'M'}, *record.parentWorld);
    }

    if (record.climate.has_value()) {
        WriteFormIDSubrecord(wrld, {'C', 'N', 'A', 'M'}, *record.climate);
    }

    if (record.water.has_value()) {
        WriteFormIDSubrecord(wrld, {'N', 'A', 'M', '2'}, *record.water);
    }

    {
        SubRecordWriter data(wrld, {'D', 'A', 'T', 'A'});
        data.WriteUInt8(worldFlags);
        data.Finalize();
    }

    // Finalize explicitly so any write failure is reported before returning.
    wrld.Finalize();
}
