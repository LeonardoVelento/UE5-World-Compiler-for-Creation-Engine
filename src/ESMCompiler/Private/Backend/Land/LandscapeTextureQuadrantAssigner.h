#pragma once

#include "LandIR.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace land_ir {

// Skyrim's four LAND texture quadrants. The canonical LandTile coordinate
// convention is X increasing right and Y increasing upward: row zero is the
// bottom of the tile. Deprecated compass aliases remain for source
// compatibility only.
enum class LandTextureQuadrant {
    BottomLeft = 0,
    BottomRight = 1,
    TopLeft = 2,
    TopRight = 3,

    SouthWest = BottomLeft,
    SouthEast = BottomRight,
    NorthWest = TopLeft,
    NorthEast = TopRight,
};

// Maps one canonical 33x33 source vertex to a local vertex index in a
// confirmed texture quadrant layout. A missing local index is intentionally
// distinguishable from index zero.
struct TextureQuadrantLocalSample {
    std::size_t sourceVertexIndex = 0;
    std::optional<std::size_t> localVertexIndex;
};

struct TextureQuadrantRule {
    LandTextureQuadrant quadrant = LandTextureQuadrant::NorthWest;

    // The confirmed Skyrim value passed later to BTXT/ATXT encoders.
    std::optional<std::uint32_t> encodedQuadrantValue;

    // The explicit spatial region, border ownership, and local indexing for
    // this quadrant. No geometric split is inferred by the compiler.
    std::vector<TextureQuadrantLocalSample> samples;
};

// expectedOwnerCounts specifies the confirmed ownership count for every
// source sample. For example, a count of one defines unique ownership; a
// count above one explicitly permits duplication when Skyrim requires it.
struct TextureQuadrantAssignmentRules {
    std::optional<std::array<TextureQuadrantRule, 4>> quadrants;
    std::optional<std::array<std::uint8_t, kPatchVertexCount>> expectedOwnerCounts;
};

// Confirmed Skyrim SE/AE texture split. Each quadrant owns a 17x17 local
// grid; samples at X=16 and/or Y=16 are intentionally duplicated between the
// adjacent quadrant grids.
[[nodiscard]] TextureQuadrantAssignmentRules MakeSkyrimSEAETextureQuadrantAssignmentRules();

struct QuadrantLocalLayerWeight {
    std::size_t sourceVertexIndex = 0;
    std::size_t localVertexIndex = 0;
    float weight = 0.0F;

    bool operator==(const QuadrantLocalLayerWeight&) const = default;
};

// A texture-layer input for exactly one source LandscapeLayer and one
// quadrant. sourceLayerIndex preserves source identity without resolving or
// looking up the LTEX record.
struct QuadrantLocalTextureLayerInput {
    std::size_t sourceLayerIndex = 0;
    LandscapeLayerRole role = LandscapeLayerRole::Additional;
    std::uint32_t ltexFormID = 0;
    LandTextureQuadrant quadrant = LandTextureQuadrant::NorthWest;
    std::uint32_t encodedQuadrantValue = 0;
    std::vector<QuadrantLocalLayerWeight> weights;

    bool operator==(const QuadrantLocalTextureLayerInput&) const = default;
};

// Produces logical, quadrant-local texture-layer inputs. It neither emits
// BTXT/ATXT/VTXT nor selects a quadrant, layer order, or local indexing.
class LandscapeTextureQuadrantAssigner final {
public:
    explicit LandscapeTextureQuadrantAssigner(
        TextureQuadrantAssignmentRules rules = {});

    [[nodiscard]] std::vector<QuadrantLocalTextureLayerInput> Assign(
        std::span<const LandscapeLayer> sourceLayers) const;

private:
    void ValidateRules() const;

    TextureQuadrantAssignmentRules rules_;
};

} // namespace land_ir
