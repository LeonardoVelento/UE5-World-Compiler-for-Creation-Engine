#pragma once

#include "BinaryWriter.h"
#include "RecordWriter.h"
#include "WRLDRecord.h"

// Serializes the binary representation of an already generated WRLD record.
// This class does not read World IR and does not create or modify record data.
class WRLDSerializer final {
public:
    // The caller supplies header policy, including the compiler-assigned FormID.
    // header.recordType must be WRLD and header.formID must be non-zero.
    void Serialize(const WRLDRecord& record,
                   const RecordHeader& header,
                   BinaryWriter& writer) const;
};
