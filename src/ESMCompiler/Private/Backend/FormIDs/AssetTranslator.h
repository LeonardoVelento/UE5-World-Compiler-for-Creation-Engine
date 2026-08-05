#pragma once

#include "WorldIR.h"

#include <cstdint>

using BaseFormID = std::uint32_t;

// Converts project AssetIDs into Creation Engine base Form IDs.
class AssetTranslator final {
public:
    // Accepts one to eight hexadecimal digits, with an optional 0x or 0X prefix.
    // Throws std::invalid_argument or std::out_of_range for an invalid AssetID.
    [[nodiscard]] BaseFormID Translate(const world_ir::AssetID& assetID) const;
};
