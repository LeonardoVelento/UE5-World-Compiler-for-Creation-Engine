#include "VtxtEntryBuilder.h"

#include "MissingSpecification.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace land_ir {

VtxtEntryBuilderRules MakeSkyrimSEAEVtxtEntryBuilderRules() {
    return {
        // Empty additional layers are omitted by the texture planner.
        .zeroOpacityHandling = VtxtZeroOpacityHandling::Omit,
    };
}

std::optional<std::size_t> SkyrimSEAEVtxtVertexIndexResolver::ResolveVertexPosition(
    const QuadrantLocalTextureLayerInput& atxtLayer,
    const QuadrantLocalLayerWeight& sample) const {
    if (atxtLayer.role != LandscapeLayerRole::Additional || sample.localVertexIndex > 288) {
        return std::nullopt;
    }
    return sample.localVertexIndex;
}

VtxtEntryBuilder::VtxtEntryBuilder(
    const IVtxtVertexIndexResolver* vertexIndexResolver,
    VtxtEntryBuilderRules rules)
    : vertexIndexResolver_(vertexIndexResolver)
    , rules_(std::move(rules)) {}

std::vector<VtxtEntry> VtxtEntryBuilder::Build(
    const QuadrantLocalTextureLayerInput& atxtLayer) const {
    ValidateDependencies();

    if (atxtLayer.role != LandscapeLayerRole::Additional) {
        throw std::invalid_argument("VTXT entry building requires an additional ATXT layer.");
    }

    std::vector<VtxtEntry> entries;
    entries.reserve(atxtLayer.weights.size());

    for (const QuadrantLocalLayerWeight& sample : atxtLayer.weights) {
        if (!std::isfinite(sample.weight)) {
            throw std::invalid_argument("VTXT entry building requires finite opacity values.");
        }
        if (sample.weight == 0.0F &&
            *rules_.zeroOpacityHandling == VtxtZeroOpacityHandling::Omit) {
            continue;
        }

        const std::optional<std::size_t> vertexPosition =
            vertexIndexResolver_->ResolveVertexPosition(atxtLayer, sample);
        if (!vertexPosition.has_value()) {
            throw MissingSpecification(
                "VTXT entry building requires a confirmed vertex position mapping for every entry.");
        }

        entries.push_back({
            .atxtLayer = {
                .sourceLayerIndex = atxtLayer.sourceLayerIndex,
                .encodedQuadrantValue = atxtLayer.encodedQuadrantValue,
            },
            .vertexPosition = *vertexPosition,
            .opacity = sample.weight,
        });
    }

    return entries;
}

void VtxtEntryBuilder::ValidateDependencies() const {
    if (vertexIndexResolver_ == nullptr) {
        throw MissingSpecification(
            "VTXT entry building requires an explicit vertex position resolver.");
    }
    if (!rules_.zeroOpacityHandling.has_value()) {
        throw MissingSpecification(
            "VTXT entry building requires an explicit zero-opacity handling rule.");
    }
}

} // namespace land_ir
