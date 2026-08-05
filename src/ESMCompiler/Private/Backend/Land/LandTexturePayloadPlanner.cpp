#include "LandTexturePayloadPlanner.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace land_ir {
namespace {

const QuadrantLocalTextureLayerInput& FindQuadrantInput(
    const std::vector<QuadrantLocalTextureLayerInput>& inputs,
    std::size_t sourceLayerIndex,
    std::uint32_t encodedQuadrantValue) {
    const auto found = std::find_if(
        inputs.begin(),
        inputs.end(),
        [sourceLayerIndex, encodedQuadrantValue](const QuadrantLocalTextureLayerInput& input) {
            return input.sourceLayerIndex == sourceLayerIndex &&
                   input.encodedQuadrantValue == encodedQuadrantValue;
        });
    if (found == inputs.end()) {
        throw std::logic_error("Texture quadrant assignment omitted a required layer input.");
    }
    return *found;
}

} // namespace

std::vector<LandTextureQuadrantPayloadGroup> SkyrimLandTexturePayloadPlanner::Plan(
    const LandTile& tile) const {
    const LandTileValidator validator;
    if (!validator.Validate(tile).empty()) {
        throw std::invalid_argument("LAND texture planning requires a valid LandTile layer collection.");
    }

    const auto baseLayerIt = std::find_if(
        tile.landscapeLayers.begin(),
        tile.landscapeLayers.end(),
        [](const LandscapeLayer& layer) { return layer.role == LandscapeLayerRole::Base; });
    if (baseLayerIt == tile.landscapeLayers.end()) {
        throw std::logic_error("Validated LandTile has no base layer.");
    }
    const std::size_t baseLayerIndex =
        static_cast<std::size_t>(baseLayerIt - tile.landscapeLayers.begin());

    const LandscapeTextureQuadrantAssigner assigner;
    const std::vector<QuadrantLocalTextureLayerInput> inputs =
        assigner.Assign(tile.landscapeLayers);

    const TextureQuadrantAssignmentRules quadrantRules =
        MakeSkyrimSEAETextureQuadrantAssignmentRules();
    const BtxtEncoder btxtEncoder;
    const AtxtEncoder atxtEncoder;
    const SkyrimSEAEVtxtVertexIndexResolver vertexIndexResolver;
    const VtxtEntryBuilder entryBuilder(
        &vertexIndexResolver,
        MakeSkyrimSEAEVtxtEntryBuilderRules());
    const VtxtEncoder vtxtEncoder;

    std::vector<LandTextureQuadrantPayloadGroup> output;
    output.reserve(quadrantRules.quadrants->size());
    for (const TextureQuadrantRule& quadrantRule : *quadrantRules.quadrants) {
        const std::uint32_t encodedQuadrantValue = *quadrantRule.encodedQuadrantValue;
        // Verify the base input exists in this confirmed 17x17 quadrant.
        static_cast<void>(FindQuadrantInput(inputs, baseLayerIndex, encodedQuadrantValue));

        LandTextureQuadrantPayloadGroup group;
        group.encodedQuadrantValue = encodedQuadrantValue;
        group.baseLayerIdentity = LandTexturePayloadIdentity{
            .sourceLayerIndex = baseLayerIndex,
            .encodedQuadrantValue = encodedQuadrantValue,
        };
        group.btxtPayload = btxtEncoder.EncodePayload(
            *baseLayerIt,
            {
                .quadrant = encodedQuadrantValue,
                .layerIndex = -1,
            });

        for (std::size_t sourceLayerIndex = 0;
             sourceLayerIndex < tile.landscapeLayers.size();
             ++sourceLayerIndex) {
            const LandscapeLayer& sourceLayer = tile.landscapeLayers[sourceLayerIndex];
            if (sourceLayer.role != LandscapeLayerRole::Additional) {
                continue;
            }

            const QuadrantLocalTextureLayerInput& alphaInput = FindQuadrantInput(
                inputs, sourceLayerIndex, encodedQuadrantValue);
            const std::vector<VtxtEntry> entries = entryBuilder.Build(alphaInput);

            // Do not write an ATXT/VTXT pair with no painted weights.
            if (entries.empty()) {
                continue;
            }

            if (group.alphaLayers.size() >
                static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max())) {
                throw std::overflow_error(
                    "LAND texture quadrant contains more ATXT layers than Skyrim's signed 16-bit Layer field supports.");
            }
            const std::int32_t layerNumber =
                static_cast<std::int32_t>(group.alphaLayers.size());

            group.alphaLayers.push_back({
                .identity = {
                    .sourceLayerIndex = sourceLayerIndex,
                    .encodedQuadrantValue = encodedQuadrantValue,
                },
                .atxtPayload = atxtEncoder.EncodePayload(
                    sourceLayer,
                    {
                        .quadrant = encodedQuadrantValue,
                        .layerIndex = layerNumber,
                    }),
                .vtxtPayload = vtxtEncoder.EncodePayload(entries),
            });
        }

        output.push_back(std::move(group));
    }

    return output;
}

} // namespace land_ir
