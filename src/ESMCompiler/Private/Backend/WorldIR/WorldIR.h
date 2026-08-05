#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace world_ir {

using AssetID = std::string;
using PersistentID = std::string;
using Metadata = std::unordered_map<std::string, std::string>;

struct Vector3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct Rotation {
    double pitch = 0.0;
    double yaw = 0.0;
    double roll = 0.0;
};

struct Color {
    float red = 0.0F;
    float green = 0.0F;
    float blue = 0.0F;
    float alpha = 1.0F;
};

struct Heightmap {
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    // Decoded Landscape local Z values in source-local units. These are not
    // native Unreal uint16 heightmap codes and have not been transformed or
    // converted into Creation Engine units.
    std::vector<double> localHeights;

    // Coordinates on the Landscape's local sample grid for localHeights[0].
    // Samples are row-major and rows increase in the positive local Y direction.
    std::int32_t firstLocalSampleX = 0;
    std::int32_t firstLocalSampleY = 0;
};

struct BlendMap {
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    // Raw authored opacity values. A BlendMap uses the same local sample-grid
    // origin and row direction as its enclosing Landscape::heightmap.
    std::vector<std::uint8_t> weights;
};

// The source-world transform of the Landscape local sample grid. It is stored
// as data only; World IR does not perform coordinate conversion with it.
struct LandscapeTransform {
    Vector3 position;
    Rotation rotation;
    Vector3 scale{1.0, 1.0, 1.0};
};

struct LandscapeBaseLayer {
    AssetID assetID;
};

struct LandscapeLayer {
    AssetID assetID;
    BlendMap blendMap;
};

struct Landscape {
    LandscapeTransform transform;
    Heightmap heightmap;
    LandscapeBaseLayer baseLayer;
    std::vector<LandscapeLayer> layers;
};

struct Object {
    PersistentID persistentID;
    AssetID assetID;
    Vector3 position;
    Rotation rotation;
    Vector3 scale{1.0, 1.0, 1.0};
    Metadata metadata;
};

struct Light {
    PersistentID persistentID;
    AssetID assetID;
    Vector3 position;
    Rotation rotation;
    Color color;
    float intensity = 0.0F;
    float radius = 0.0F;
    Metadata metadata;
};

struct World {
    std::string name;
    std::string displayName;
    Landscape landscape;
    std::vector<Object> objects;
    std::vector<Light> lights;
    Metadata metadata;
};

} // namespace world_ir
