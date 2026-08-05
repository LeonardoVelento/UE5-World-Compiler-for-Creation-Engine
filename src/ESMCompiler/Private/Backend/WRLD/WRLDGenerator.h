#pragma once

#include "WRLDRecord.h"
#include "WorldIR.h"

// Translates the world-level portion of World IR into a logical WRLD record.
class WRLDGenerator final {
public:
    [[nodiscard]] WRLDRecord Generate(const world_ir::World& world) const;
};
