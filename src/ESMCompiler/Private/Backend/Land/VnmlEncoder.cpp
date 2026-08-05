#include "VnmlEncoder.h"

#include "MissingSpecification.h"

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace land_ir {
namespace {

constexpr std::array<char, 4> kVnmlSignature{'V', 'N', 'M', 'L'};
constexpr std::size_t kVnmlComponentCount = 3;
constexpr std::size_t kVnmlPayloadSize =
    NormalPatch::VertexCount * kVnmlComponentCount;
constexpr double kNormalLengthTolerance = 1e-6;

double ReadAxis(const TerrainNormal& normal, VnmlAxis axis) {
    switch (axis) {
    case VnmlAxis::X:
        return normal.x;
    case VnmlAxis::Y:
        return normal.y;
    case VnmlAxis::Z:
        return normal.z;
    }

    throw std::invalid_argument("VNML axis order contains an invalid axis.");
}

double ApplyRounding(double value, VnmlRoundingMode roundingMode) {
    switch (roundingMode) {
    case VnmlRoundingMode::Nearest:
        return std::round(value);
    case VnmlRoundingMode::Floor:
        return std::floor(value);
    case VnmlRoundingMode::Ceiling:
        return std::ceil(value);
    case VnmlRoundingMode::Truncate:
        return std::trunc(value);
    }

    throw std::invalid_argument("VNML quantization contains an invalid rounding mode.");
}

void ValidateNormal(const TerrainNormal& normal) {
    if (!std::isfinite(normal.x) || !std::isfinite(normal.y) || !std::isfinite(normal.z)) {
        throw std::invalid_argument("VNML encoding requires finite normal components.");
    }

    const double length = std::hypot(normal.x, normal.y, normal.z);
    if (!std::isfinite(length) || std::abs(length - 1.0) > kNormalLengthTolerance) {
        throw std::invalid_argument("VNML encoding requires approximately unit-length normals.");
    }
}

} // namespace

VnmlEncodingLayout MakeSkyrimSEAEVnmlEncodingLayout() {
    VnmlVertexOrder vertexOrder;
    for (std::size_t index = 0; index < vertexOrder.sourceIndices.size(); ++index) {
        vertexOrder.sourceIndices[index] = index;
    }

    return {
        .quantization = VnmlQuantizationSpecification{
            .storage = VnmlComponentStorage::SignedInt8,
            .scale = 127.0,
            .offset = 0.0,
            .roundingMode = VnmlRoundingMode::Nearest,
        },
        .axisOrder = std::array<VnmlAxis, 3>{VnmlAxis::X, VnmlAxis::Y, VnmlAxis::Z},
        .vertexOrder = vertexOrder,
    };
}

VnmlEncoder::VnmlEncoder(VnmlEncodingLayout layout)
    : layout_(std::move(layout)) {}

LandSubrecordType VnmlEncoder::Type() const noexcept {
    return LandSubrecordType::Vnml;
}

EncodedLandSubrecord VnmlEncoder::Encode(const LandTile& tile) const {
    return {
        .type = kVnmlSignature,
        .payload = EncodePayload(tile.normalPatch),
    };
}

std::vector<std::uint8_t> VnmlEncoder::EncodePayload(const NormalPatch& patch) const {
    std::array<TerrainNormal, NormalPatch::VertexCount> normals;
    for (std::size_t index = 0; index < patch.normals.size(); ++index) {
        if (!patch.normals[index].has_value()) {
            throw std::invalid_argument("VNML encoding requires all 1089 normals to be present.");
        }
        normals[index] = *patch.normals[index];
    }

    return EncodePayload(std::span<const TerrainNormal>(normals));
}

std::vector<std::uint8_t> VnmlEncoder::EncodePayload(
    std::span<const TerrainNormal> normals) const {
    if (normals.size() != NormalPatch::VertexCount) {
        throw std::invalid_argument("VNML encoding requires exactly 1089 normals.");
    }

    for (const TerrainNormal& normal : normals) {
        ValidateNormal(normal);
    }
    ValidateLayout();

    std::vector<std::uint8_t> payload;
    payload.reserve(kVnmlPayloadSize);
    for (const std::size_t sourceIndex : layout_.vertexOrder->sourceIndices) {
        const TerrainNormal& normal = normals[sourceIndex];
        for (const VnmlAxis axis : *layout_.axisOrder) {
            payload.push_back(Quantize(ReadAxis(normal, axis)));
        }
    }

    return payload;
}

void VnmlEncoder::WritePayload(const NormalPatch& patch, BinaryWriter& writer) const {
    const std::vector<std::uint8_t> payload = EncodePayload(patch);
    writer.WriteBytes(payload.data(), payload.size());
}

void VnmlEncoder::ValidateLayout() const {
    if (!layout_.quantization.has_value() || !layout_.axisOrder.has_value() ||
        !layout_.vertexOrder.has_value()) {
        throw MissingSpecification(
            "VNML encoding requires explicitly supplied quantization, axis, and vertex orderings.");
    }

    const VnmlQuantizationSpecification& quantization = *layout_.quantization;
    if (!std::isfinite(quantization.scale) || !std::isfinite(quantization.offset) ||
        quantization.scale <= 0.0) {
        throw std::invalid_argument(
            "VNML quantization requires a finite, positive scale and a finite offset.");
    }

    std::array<bool, kVnmlComponentCount> seenAxes{};
    for (const VnmlAxis axis : *layout_.axisOrder) {
        const std::size_t axisIndex = static_cast<std::size_t>(axis);
        if (axisIndex >= seenAxes.size() || seenAxes[axisIndex]) {
            throw std::invalid_argument("VNML axis order must be a permutation of X, Y, and Z.");
        }
        seenAxes[axisIndex] = true;
    }

    std::array<bool, NormalPatch::VertexCount> seenVertices{};
    for (const std::size_t sourceIndex : layout_.vertexOrder->sourceIndices) {
        if (sourceIndex >= seenVertices.size() || seenVertices[sourceIndex]) {
            throw std::invalid_argument(
                "VNML vertex order must be a permutation of all 1089 vertex indices.");
        }
        seenVertices[sourceIndex] = true;
    }
}

std::uint8_t VnmlEncoder::Quantize(double component) const {
    const VnmlQuantizationSpecification& quantization = *layout_.quantization;
    const double rounded = ApplyRounding(
        component * quantization.scale + quantization.offset,
        quantization.roundingMode);

    if (quantization.storage == VnmlComponentStorage::SignedInt8) {
        if (rounded < std::numeric_limits<std::int8_t>::min() ||
            rounded > std::numeric_limits<std::int8_t>::max()) {
            throw std::out_of_range("VNML signed component exceeds int8 range.");
        }
        return static_cast<std::uint8_t>(static_cast<std::int8_t>(rounded));
    }

    if (quantization.storage == VnmlComponentStorage::UnsignedInt8) {
        if (rounded < std::numeric_limits<std::uint8_t>::min() ||
            rounded > std::numeric_limits<std::uint8_t>::max()) {
            throw std::out_of_range("VNML unsigned component exceeds uint8 range.");
        }
        return static_cast<std::uint8_t>(rounded);
    }

    throw std::invalid_argument("VNML quantization contains an invalid component storage type.");
}

} // namespace land_ir
