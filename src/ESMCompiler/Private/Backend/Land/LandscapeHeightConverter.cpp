#include "LandscapeHeightConverter.h"

namespace land_ir {

HeightPatch LandscapeHeightConverter::Convert(
    const RawLandscapeHeightPatch& unrealHeightPatch) const {
    HeightPatch creationHeightPatch;

    for (std::size_t index = 0; index < HeightPatch::VertexCount; ++index) {
        const std::optional<double>& unrealHeight = unrealHeightPatch.heights[index];
        if (unrealHeight.has_value()) {
            creationHeightPatch.heights[index] =
                *unrealHeight / kUnrealUnitsPerCreationGameUnit;
        }
    }

    return creationHeightPatch;
}

} // namespace land_ir
