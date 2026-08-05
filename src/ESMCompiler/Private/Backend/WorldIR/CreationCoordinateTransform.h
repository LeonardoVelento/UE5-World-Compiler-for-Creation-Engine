#pragma once

#include "WorldIR.h"

// Configuration for conversion from Unreal world-space units to Creation
// world-space units. unitScale is deliberately supplied by the caller.
struct CreationTransformSettings {
    double unitScale;
    bool invertYAxis = true;
    world_ir::Vector3 worldOriginOffset;
};

// Converts only a world position. CELL assignment and rotation conversion are
// intentionally separate compiler stages.
[[nodiscard]] world_ir::Vector3 ToCreationPosition(
    const world_ir::Vector3& unrealPosition,
    const CreationTransformSettings& settings);
