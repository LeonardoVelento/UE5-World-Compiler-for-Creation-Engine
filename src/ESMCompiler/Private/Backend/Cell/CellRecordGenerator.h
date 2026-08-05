#pragma once

#include "CellGrid.h"
#include "ExteriorCellRecord.h"
#include "IFormIDAllocator.h"

#include <vector>

// Translates a logical cell grid into logical exterior CELL records.
// It does not serialize records or create LAND/REFR data.
class CellRecordGenerator final {
public:
    [[nodiscard]] std::vector<ExteriorCellRecord> Generate(
        const CellGrid& grid,
        IFormIDAllocator& formIDAllocator) const;
};
