#pragma once

#include "BinaryWriter.h"
#include "LandSubrecordEncoder.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace land_ir {

enum class VnmlAxis {
    X,
    Y,
    Z,
};

enum class VnmlComponentStorage {
    SignedInt8,
    UnsignedInt8,
};

enum class VnmlRoundingMode {
    Nearest,
    Floor,
    Ceiling,
    Truncate,
};

// The supplied values define how a normal component becomes a single byte.
// offset and roundingMode are explicit so unsigned encodings and rounding
// behavior are never guessed by the compiler.
struct VnmlQuantizationSpecification {
    VnmlComponentStorage storage = VnmlComponentStorage::SignedInt8;
    double scale = 0.0;
    double offset = 0.0;
    VnmlRoundingMode roundingMode = VnmlRoundingMode::Nearest;
};

struct VnmlVertexOrder {
    std::array<std::size_t, NormalPatch::VertexCount> sourceIndices{};
};

// Explicit layouts remain supported for tests or a future game target.
struct VnmlEncodingLayout {
    std::optional<VnmlQuantizationSpecification> quantization;
    std::optional<std::array<VnmlAxis, 3>> axisOrder;
    std::optional<VnmlVertexOrder> vertexOrder;
};

// Confirmed Skyrim SE/AE VNML convention: 1089 row-major normals, emitted as
// signed int8 X/Y/Z components using round(component * 127). The byte stream
// stores the two's-complement representation of those signed values.
[[nodiscard]] VnmlEncodingLayout MakeSkyrimSEAEVnmlEncodingLayout();

// Encodes only the VNML payload. SubrecordWriter owns the VNML signature and
// payload-size field; this class never emits a subrecord header.
class VnmlEncoder final : public ILandVnmlEncoder {
public:
    explicit VnmlEncoder(
        VnmlEncodingLayout layout = MakeSkyrimSEAEVnmlEncodingLayout());

    [[nodiscard]] LandSubrecordType Type() const noexcept override;
    [[nodiscard]] EncodedLandSubrecord Encode(const LandTile& tile) const override;

    [[nodiscard]] std::vector<std::uint8_t> EncodePayload(const NormalPatch& patch) const;
    [[nodiscard]] std::vector<std::uint8_t> EncodePayload(
        std::span<const TerrainNormal> normals) const;

    // Writes raw VNML payload bytes only after every validation succeeds.
    void WritePayload(const NormalPatch& patch, BinaryWriter& writer) const;

private:
    void ValidateLayout() const;
    [[nodiscard]] std::uint8_t Quantize(double component) const;

    VnmlEncodingLayout layout_;
};

} // namespace land_ir
