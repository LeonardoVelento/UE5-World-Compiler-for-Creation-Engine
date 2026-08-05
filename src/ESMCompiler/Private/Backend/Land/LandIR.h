#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

// Engine-independent logical terrain data. This header deliberately contains
// no Unreal Engine types, Creation Engine record types, or binary layouts.
namespace land_ir {

inline constexpr std::size_t kPatchVertexSide = 33;
inline constexpr std::size_t kPatchVertexCount = kPatchVertexSide * kPatchVertexSide;

struct LandCellCoordinates {
    std::int32_t x = 0;
    std::int32_t y = 0;

    bool operator==(const LandCellCoordinates&) const = default;
};

struct HeightPatch {
    static constexpr std::size_t VertexSide = kPatchVertexSide;
    static constexpr std::size_t VertexCount = kPatchVertexCount;

    // Heights are Creation Engine game-unit values in logical, unencoded form.
    // A disengaged value is a source vertex that remains explicitly missing.
    std::array<std::optional<double>, VertexCount> heights{};
};

struct TerrainNormal {
    double x = 0.0;
    double y = 0.0;
    double z = 1.0;
};

struct NormalPatch {
    static constexpr std::size_t VertexSide = kPatchVertexSide;
    static constexpr std::size_t VertexCount = kPatchVertexCount;

    // Floating-point geometric normals. A disengaged value means terrain
    // geometry could not be derived because the necessary height data was
    // missing. VNML quantization is intentionally outside Land IR.
    std::array<std::optional<TerrainNormal>, VertexCount> normals{};
};

struct RGBVertexColor {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

struct VertexColorPatch {
    static constexpr std::size_t VertexSide = kPatchVertexSide;
    static constexpr std::size_t VertexCount = kPatchVertexCount;

    // Logical RGB colours only. VCLR channel ordering is not represented here.
    std::array<RGBVertexColor, VertexCount> colors{};
};

struct LayerWeightPatch {
    static constexpr std::size_t VertexSide = kPatchVertexSide;
    static constexpr std::size_t VertexCount = kPatchVertexCount;

    // Logical normalized opacity values in the inclusive [0, 1] range. VTXT
    // positions and quadrants are not represented in Land IR.
    std::array<float, VertexCount> weights{};
};

enum class LandscapeLayerRole {
    Base,
    Additional,
};

struct LandscapeLayer {
    LandscapeLayerRole role = LandscapeLayerRole::Additional;

    // A project-supplied identifier. Zero intentionally represents a missing
    // assignment and is reported by LandTileValidator.
    std::uint32_t ltexFormID = 0;

    LayerWeightPatch blendOpacity;
};

struct LandTile {
    LandCellCoordinates exteriorCellCoordinates;
    HeightPatch heightPatch;
    NormalPatch normalPatch;
    VertexColorPatch vertexColorPatch;
    std::vector<LandscapeLayer> landscapeLayers;
};

enum class LandTileValidationErrorCode {
    MissingBaseLayer,
    MultipleBaseLayers,
    MissingLtexFormID,
};

struct LandTileValidationError {
    LandTileValidationErrorCode code;
    std::optional<std::size_t> layerIndex;
};

// Performs only Land IR structural validation. It does not look up plugins,
// resolve LTEX records, encode LAND subrecords, or modify the tile.
class LandTileValidator final {
public:
    [[nodiscard]] std::vector<LandTileValidationError> Validate(const LandTile& tile) const;
};

} // namespace land_ir
