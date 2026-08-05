#pragma once

#include "CellGrid.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Compiler-side logical model of one exterior CELL record.
// It stores only record identities and relationships, never binary payloads.
struct ExteriorCellRecord {
    std::uint32_t formID;
    CellCoordinates coordinates;

    std::string editorID;

    std::uint16_t flags = 0;

    bool containsLandscape = false;

    std::vector<std::uint32_t> temporaryReferenceFormIDs;
    std::vector<std::uint32_t> persistentReferenceFormIDs;

    std::optional<std::uint32_t> landFormID;
};
