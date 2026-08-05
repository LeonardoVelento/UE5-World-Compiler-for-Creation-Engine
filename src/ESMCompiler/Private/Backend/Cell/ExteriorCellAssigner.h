#pragma once

#include "CellCoordinateCalculator.h"
#include "CellGrid.h"
#include "CreationCoordinateTransform.h"
#include "WorldIR.h"

// Builds exterior CELL membership from World IR positions. This function does
// not modify World IR and does not generate any Creation Engine records.
[[nodiscard]] CellGrid BuildCellGrid(
    const world_ir::World& world,
    const CreationTransformSettings& transformSettings,
    const CellCoordinateCalculator& calculator);
