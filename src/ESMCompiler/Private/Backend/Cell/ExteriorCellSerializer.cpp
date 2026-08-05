#include "ExteriorCellSerializer.h"

#include "RecordWriter.h"
#include "SubRecordWriter.h"

#include <cstdint>

namespace {

constexpr std::uint16_t kInteriorCellFlag = 0x0001;
constexpr std::uint32_t kForceHideLandFlags = 0;

} // namespace

void ExteriorCellSerializer::Serialize(const ExteriorCellRecord& record,
                                       BinaryWriter& writer) const {
    RecordHeader header{};
    header.recordType = {'C', 'E', 'L', 'L'};
    header.formID = record.formID;

    RecordWriter cell(writer, header);

    {
        SubRecordWriter data(cell, {'D', 'A', 'T', 'A'});

        // An exterior CELL is represented by the absence of the Interior flag.
        // All other logical flag bits are preserved exactly as supplied.
        const std::uint16_t exteriorFlags =
            static_cast<std::uint16_t>(record.flags & ~kInteriorCellFlag);
        data.WriteUInt16(exteriorFlags);
        data.Finalize();
    }

    {
        SubRecordWriter xclc(cell, {'X', 'C', 'L', 'C'});
        xclc.WriteInt32(record.coordinates.x);
        xclc.WriteInt32(record.coordinates.y);
        xclc.WriteUInt32(kForceHideLandFlags);
        xclc.Finalize();
    }

    cell.Finalize();
}
