#pragma once

#include "CellGrid.h"
#include "LandIR.h"
#include "WorldIR.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

// Logical RGB colour only. VCLR byte ordering remains a serializer concern.
struct ColorRGB8 {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

struct LandVertex {
    double height = 0.0;
    world_ir::Vector3 normal{0.0, 0.0, 1.0};
    ColorRGB8 color{};
};

enum class LandscapeLayerRole {
    Base,
    Additional,
};

// Canonical full-LAND texture input. The weights use the compiler's logical
// 33x33 vertex indexing; no Skyrim quadrant or VTXT position is implied here.
struct LandscapeLayerInput {
    static constexpr std::size_t VertexSide = 33;
    static constexpr std::size_t VertexCount = VertexSide * VertexSide;

    std::uint32_t ltexFormID = 0;
    LandscapeLayerRole role = LandscapeLayerRole::Additional;
    std::array<float, VertexCount> weights{};
};

// Deliberately contains source texture inputs only. The later
// LandTexturePlanner is the sole owner of quadrant, layer-index, BTXT, ATXT
// and VTXT planning.
struct LogicalLandTextureInputs {
    std::optional<LandscapeLayerInput> baseLayer;
    std::vector<LandscapeLayerInput> additionalLayers;
};

// Compiler-side logical LAND record. It contains no binary encoding and no
// Skyrim subrecord layout decisions.
struct LandRecord {
    static constexpr std::size_t VertexSide = 33;
    static constexpr std::size_t VertexCount = VertexSide * VertexSide;

    std::uint32_t formID = 0;
    std::uint32_t owningCellFormID = 0;
    CellCoordinates cellCoordinates{};

    // Completed engine-independent terrain payload. It is optional while LAND
    // generation is incomplete; a serializer must reject an absent payload.
    std::optional<land_ir::LandTile> landTile;

    // Canonical in-memory order is localY * VertexSide + localX. This is not
    // a declaration of any later Skyrim binary ordering.
    std::array<LandVertex, VertexCount> vertices{};
    LogicalLandTextureInputs textureInputs;
};
