#pragma once

#include "IWorldReader.h"

// Unreal Engine 5 adapter that extracts authoring data into World IR.
class UEWorldReader final : public IWorldReader {
public:
    [[nodiscard]] world_ir::World Read(UWorld* world) override;
};
