#pragma once

#include "LandRecord.h"

#include <cstdint>
#include <optional>
#include <vector>

enum class ReferencePlacement {
    Persistent,
    Temporary,
    VisibleDistant,
};

// Minimal logical REFR identity used for record-group planning.
struct ReferenceRecord {
    std::uint32_t formID;
    CellCoordinates cellCoordinates;
    ReferencePlacement placement;
};

struct CellChildGroupPlan {
    std::uint32_t cellFormID;

    std::vector<std::uint32_t> persistentReferenceFormIDs;
    std::vector<std::uint32_t> temporaryReferenceFormIDs;
    std::vector<std::uint32_t> visibleDistantReferenceFormIDs;

    std::optional<std::uint32_t> landFormID;
};

struct ExteriorCellGroupPlan {
    CellCoordinates coordinates;
    std::uint32_t cellFormID;
    CellChildGroupPlan children;
};

struct WorldspaceGroupPlan {
    std::uint32_t worldFormID;
    std::vector<ExteriorCellGroupPlan> exteriorCells;
};
