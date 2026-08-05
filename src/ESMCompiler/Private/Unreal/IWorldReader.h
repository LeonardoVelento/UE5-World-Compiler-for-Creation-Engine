#pragma once

#include "../Backend/WorldIR/WorldIR.h"

class UWorld;

class IWorldReader {
public:
    virtual ~IWorldReader() = default;

    [[nodiscard]] virtual world_ir::World Read(UWorld* world) = 0;
};
