#pragma once

#include "CellGrid.h"
#include "ExteriorCellRecord.h"
#include "LandRecord.h"

#include <vector>

// Links already-generated logical LAND records to logical exterior CELL
// records. REFR linking is not part of the current MVP and will be introduced
// together with REFR generation.
class CellContentLinker final {
public:
    void Link(const CellGrid& grid,
              std::vector<ExteriorCellRecord>& cellRecords,
              const std::vector<LandRecord>& landRecords) const;
};
