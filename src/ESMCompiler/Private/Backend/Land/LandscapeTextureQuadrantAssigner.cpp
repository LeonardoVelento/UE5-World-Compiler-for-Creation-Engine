#include "LandscapeTextureQuadrantAssigner.h"

#include "MissingSpecification.h"

#include <array>
#include <stdexcept>
#include <utility>

namespace land_ir {
namespace {

constexpr std::size_t kQuadrantCount = 4;
constexpr std::size_t kQuadrantVertexSide = 17;
constexpr std::size_t kQuadrantVertexCount = kQuadrantVertexSide * kQuadrantVertexSide;

std::size_t ToIndex(LandTextureQuadrant quadrant) {
    return static_cast<std::size_t>(quadrant);
}

} // namespace

TextureQuadrantAssignmentRules MakeSkyrimSEAETextureQuadrantAssignmentRules() {
    std::array<TextureQuadrantRule, kQuadrantCount> quadrants{
        TextureQuadrantRule{
            .quadrant = LandTextureQuadrant::BottomLeft,
            .encodedQuadrantValue = 0,
        },
        TextureQuadrantRule{
            .quadrant = LandTextureQuadrant::BottomRight,
            .encodedQuadrantValue = 1,
        },
        TextureQuadrantRule{
            .quadrant = LandTextureQuadrant::TopLeft,
            .encodedQuadrantValue = 2,
        },
        TextureQuadrantRule{
            .quadrant = LandTextureQuadrant::TopRight,
            .encodedQuadrantValue = 3,
        },
    };

    for (std::size_t quadrantIndex = 0; quadrantIndex < quadrants.size(); ++quadrantIndex) {
        const bool right = quadrantIndex == 1 || quadrantIndex == 3;
        const bool top = quadrantIndex == 2 || quadrantIndex == 3;
        const std::size_t firstX = right ? 16 : 0;
        const std::size_t firstY = top ? 16 : 0;

        quadrants[quadrantIndex].samples.reserve(kQuadrantVertexCount);
        for (std::size_t localY = 0; localY < kQuadrantVertexSide; ++localY) {
            for (std::size_t localX = 0; localX < kQuadrantVertexSide; ++localX) {
                const std::size_t sourceX = firstX + localX;
                const std::size_t sourceY = firstY + localY;
                quadrants[quadrantIndex].samples.push_back({
                    .sourceVertexIndex = sourceY * kPatchVertexSide + sourceX,
                    .localVertexIndex = localY * kQuadrantVertexSide + localX,
                });
            }
        }
    }

    std::array<std::uint8_t, kPatchVertexCount> ownershipCounts{};
    for (std::size_t y = 0; y < kPatchVertexSide; ++y) {
        for (std::size_t x = 0; x < kPatchVertexSide; ++x) {
            const bool verticalBorder = x == 16;
            const bool horizontalBorder = y == 16;
            ownershipCounts[y * kPatchVertexSide + x] =
                static_cast<std::uint8_t>((verticalBorder ? 2 : 1) *
                                          (horizontalBorder ? 2 : 1));
        }
    }

    return {
        .quadrants = std::move(quadrants),
        .expectedOwnerCounts = ownershipCounts,
    };
}

LandscapeTextureQuadrantAssigner::LandscapeTextureQuadrantAssigner(
    TextureQuadrantAssignmentRules rules)
    : rules_(rules.quadrants.has_value() || rules.expectedOwnerCounts.has_value()
                 ? std::move(rules)
                 : MakeSkyrimSEAETextureQuadrantAssignmentRules()) {}

std::vector<QuadrantLocalTextureLayerInput> LandscapeTextureQuadrantAssigner::Assign(
    std::span<const LandscapeLayer> sourceLayers) const {
    ValidateRules();

    const auto& quadrantRules = *rules_.quadrants;
    std::vector<QuadrantLocalTextureLayerInput> inputs;
    inputs.reserve(sourceLayers.size() * quadrantRules.size());

    for (std::size_t sourceLayerIndex = 0; sourceLayerIndex < sourceLayers.size();
         ++sourceLayerIndex) {
        const LandscapeLayer& sourceLayer = sourceLayers[sourceLayerIndex];
        for (const TextureQuadrantRule& quadrantRule : quadrantRules) {
            QuadrantLocalTextureLayerInput input;
            input.sourceLayerIndex = sourceLayerIndex;
            input.role = sourceLayer.role;
            input.ltexFormID = sourceLayer.ltexFormID;
            input.quadrant = quadrantRule.quadrant;
            input.encodedQuadrantValue = *quadrantRule.encodedQuadrantValue;
            input.weights.reserve(quadrantRule.samples.size());

            for (const TextureQuadrantLocalSample& sample : quadrantRule.samples) {
                input.weights.push_back({
                    .sourceVertexIndex = sample.sourceVertexIndex,
                    .localVertexIndex = *sample.localVertexIndex,
                    .weight = sourceLayer.blendOpacity.weights[sample.sourceVertexIndex],
                });
            }

            inputs.push_back(std::move(input));
        }
    }

    return inputs;
}

void LandscapeTextureQuadrantAssigner::ValidateRules() const {
    if (!rules_.quadrants.has_value()) {
        throw MissingSpecification(
            "Landscape texture quadrant assignment requires confirmed quadrant regions and values.");
    }
    if (!rules_.expectedOwnerCounts.has_value()) {
        throw MissingSpecification(
            "Landscape texture quadrant assignment requires confirmed border ownership rules.");
    }

    const auto& quadrantRules = *rules_.quadrants;
    std::array<bool, kQuadrantCount> seenQuadrants{};
    std::array<std::uint32_t, kQuadrantCount> encodedValues{};
    std::size_t encodedValueCount = 0;
    std::array<std::size_t, kPatchVertexCount> actualOwnerCounts{};

    for (const TextureQuadrantRule& quadrantRule : quadrantRules) {
        const std::size_t quadrantIndex = ToIndex(quadrantRule.quadrant);
        if (quadrantIndex >= seenQuadrants.size() || seenQuadrants[quadrantIndex]) {
            throw std::invalid_argument(
                "Texture quadrant rules must contain each semantic quadrant exactly once.");
        }
        seenQuadrants[quadrantIndex] = true;

        if (!quadrantRule.encodedQuadrantValue.has_value()) {
            throw MissingSpecification(
                "Landscape texture quadrant assignment requires confirmed encoded quadrant values.");
        }
        for (std::size_t index = 0; index < encodedValueCount; ++index) {
            if (encodedValues[index] == *quadrantRule.encodedQuadrantValue) {
                throw std::invalid_argument("Texture quadrant rules contain duplicate encoded values.");
            }
        }
        encodedValues[encodedValueCount++] = *quadrantRule.encodedQuadrantValue;

        if (quadrantRule.samples.size() != kQuadrantVertexCount) {
            throw std::invalid_argument(
                "Each Skyrim texture quadrant must contain exactly 17x17 local samples.");
        }
        std::array<bool, kQuadrantVertexCount> seenLocalIndices{};

        for (const TextureQuadrantLocalSample& sample : quadrantRule.samples) {
            if (sample.sourceVertexIndex >= kPatchVertexCount) {
                throw std::invalid_argument(
                    "Texture quadrant rule references a source vertex outside the 33x33 patch.");
            }
            if (!sample.localVertexIndex.has_value()) {
                throw MissingSpecification(
                    "Landscape texture quadrant assignment requires confirmed local vertex indices.");
            }
            if (*sample.localVertexIndex >= kQuadrantVertexCount ||
                seenLocalIndices[*sample.localVertexIndex]) {
                throw std::invalid_argument(
                    "Texture quadrant local indices must be a unique 17x17 grid.");
            }
            seenLocalIndices[*sample.localVertexIndex] = true;
            ++actualOwnerCounts[sample.sourceVertexIndex];
        }
    }

    for (std::size_t sourceVertexIndex = 0; sourceVertexIndex < actualOwnerCounts.size();
         ++sourceVertexIndex) {
        if (actualOwnerCounts[sourceVertexIndex] !=
            (*rules_.expectedOwnerCounts)[sourceVertexIndex]) {
            throw std::invalid_argument(
                "Texture quadrant sample ownership does not match the confirmed rules.");
        }
    }
}

} // namespace land_ir
