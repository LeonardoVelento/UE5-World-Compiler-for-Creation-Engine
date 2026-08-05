#pragma once

#include "CanonicalLandscapeSampler.h"

namespace land_ir {

// One Creation Engine game unit is this many Unreal units.
inline constexpr double kUnrealUnitsPerCreationGameUnit = 1.42875;

// Converts logical height values only. It does not encode VHGT, alter sample
// ordering, or synthesize values for missing samples.
class LandscapeHeightConverter final {
public:
    [[nodiscard]] HeightPatch Convert(const RawLandscapeHeightPatch& unrealHeightPatch) const;
};

} // namespace land_ir
