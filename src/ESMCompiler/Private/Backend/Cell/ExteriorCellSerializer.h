#pragma once

#include "BinaryWriter.h"
#include "ExteriorCellRecord.h"

// Serializes the MVP binary representation of one logical exterior CELL.
// Only DATA and XCLC are written.
class ExteriorCellSerializer final {
public:
    void Serialize(const ExteriorCellRecord& record, BinaryWriter& writer) const;
};
