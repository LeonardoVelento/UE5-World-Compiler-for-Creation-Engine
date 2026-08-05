#pragma once

#include "ExteriorCellRecord.h"
#include "WorldspaceGroupPlan.h"

#include <cstdint>
#include <vector>

// Builds a logical hierarchy for generated records. It does not use Creation
// Engine GRUP type values and does not serialize any data.
class WorldspaceGroupPlanner final {
public:
    [[nodiscard]] WorldspaceGroupPlan Build(
        std::uint32_t worldFormID,
        const std::vector<ExteriorCellRecord>& cellRecords,
        const std::vector<LandRecord>& landRecords,
        const std::vector<ReferenceRecord>& referenceRecords) const;
};
