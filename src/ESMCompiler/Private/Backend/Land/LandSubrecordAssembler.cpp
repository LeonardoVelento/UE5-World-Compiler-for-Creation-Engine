#include "LandSubrecordAssembler.h"

#include "MissingSpecification.h"
#include "SubRecordWriter.h"

#include <array>
#include <stdexcept>
#include <string>
#include <utility>

namespace land_ir {
namespace {

constexpr std::size_t kTopLevelItemCount = 5;
constexpr std::size_t kTextureGroupItemCount = 2;
constexpr std::size_t kAlphaLayerItemCount = 2;

std::array<char, 4> ToSignature(LandSubrecordType type) {
    switch (type) {
    case LandSubrecordType::Data:
        return {'D', 'A', 'T', 'A'};
    case LandSubrecordType::Vhgt:
        return {'V', 'H', 'G', 'T'};
    case LandSubrecordType::Vnml:
        return {'V', 'N', 'M', 'L'};
    case LandSubrecordType::Vclr:
        return {'V', 'C', 'L', 'R'};
    case LandSubrecordType::Btxt:
        return {'B', 'T', 'X', 'T'};
    case LandSubrecordType::Atxt:
        return {'A', 'T', 'X', 'T'};
    case LandSubrecordType::Vtxt:
        return {'V', 'T', 'X', 'T'};
    }

    throw std::invalid_argument("LAND assembly encountered an unknown subrecord type.");
}

const char* ToString(LandSubrecordType type) {
    switch (type) {
    case LandSubrecordType::Data:
        return "DATA";
    case LandSubrecordType::Vhgt:
        return "VHGT";
    case LandSubrecordType::Vnml:
        return "VNML";
    case LandSubrecordType::Vclr:
        return "VCLR";
    case LandSubrecordType::Btxt:
        return "BTXT";
    case LandSubrecordType::Atxt:
        return "ATXT";
    case LandSubrecordType::Vtxt:
        return "VTXT";
    }

    return "unknown";
}

void ValidateRequirement(
    const std::optional<std::vector<std::uint8_t>>& payload,
    LandPayloadRequirement requirement,
    const char* name) {
    if (requirement == LandPayloadRequirement::Required && !payload.has_value()) {
        throw std::invalid_argument(std::string("LAND assembly requires ") + name + " payload.");
    }
    if (requirement == LandPayloadRequirement::Forbidden && payload.has_value()) {
        throw std::invalid_argument(std::string("LAND assembly forbids ") + name + " payload.");
    }
}

void AppendPayload(
    std::vector<AssembledLandSubrecord>& output,
    LandSubrecordType type,
    const std::optional<std::vector<std::uint8_t>>& payload,
    std::optional<LandTexturePayloadIdentity> textureIdentity = std::nullopt) {
    if (!payload.has_value()) {
        return;
    }
    output.push_back({
        .type = type,
        .payload = *payload,
        .textureIdentity = textureIdentity,
    });
}

template <typename OrderItem, std::size_t Count>
void ValidatePermutation(const std::vector<OrderItem>& order, const char* name) {
    if (order.size() != Count) {
        throw std::invalid_argument(std::string("LAND ") + name + " must contain every item exactly once.");
    }

    std::array<bool, Count> seen{};
    for (const OrderItem item : order) {
        const std::size_t index = static_cast<std::size_t>(item);
        if (index >= seen.size() || seen[index]) {
            throw std::invalid_argument(std::string("LAND ") + name + " is not a valid permutation.");
        }
        seen[index] = true;
    }
}

} // namespace

LandSubrecordAssembler::LandSubrecordAssembler(LandSubrecordAssemblyRules rules)
    : rules_(std::move(rules)) {}

LandSubrecordAssemblyRules MakeSkyrimSEAELandSubrecordAssemblyRules() {
    return {
        .topLevelOrder = std::vector<LandTopLevelOrderItem>{
            LandTopLevelOrderItem::Data,
            LandTopLevelOrderItem::Vnml,
            LandTopLevelOrderItem::Vhgt,
            LandTopLevelOrderItem::Vclr,
            LandTopLevelOrderItem::TextureQuadrants,
        },
        .textureGroupOrder = std::vector<LandTextureGroupOrderItem>{
            LandTextureGroupOrderItem::Btxt,
            LandTextureGroupOrderItem::AlphaLayers,
        },
        .alphaLayerOrder = std::vector<LandAlphaLayerOrderItem>{
            LandAlphaLayerOrderItem::Atxt,
            LandAlphaLayerOrderItem::Vtxt,
        },
        .presenceRequirements = LandSubrecordPresenceRequirements{
            .data = LandPayloadRequirement::Required,
            .vhgt = LandPayloadRequirement::Required,
            .vnml = LandPayloadRequirement::Required,
            .vclr = LandPayloadRequirement::Required,
            .textureQuadrants = LandPayloadRequirement::Required,
            .btxtPerTextureQuadrant = LandPayloadRequirement::Required,
            .atxtPerAlphaLayer = LandPayloadRequirement::Required,
            .vtxtPerAlphaLayer = LandPayloadRequirement::Required,
        },
    };
}

std::vector<AssembledLandSubrecord> LandSubrecordAssembler::Assemble(
    const LandSubrecordAssemblyInput& input) const {
    ValidateRules();

    const LandSubrecordPresenceRequirements& requirements = *rules_.presenceRequirements;
    ValidateRequirement(input.dataPayload, requirements.data, "DATA");
    ValidateRequirement(input.vhgtPayload, requirements.vhgt, "VHGT");
    ValidateRequirement(input.vnmlPayload, requirements.vnml, "VNML");
    ValidateRequirement(input.vclrPayload, requirements.vclr, "VCLR");

    const bool hasTextureQuadrants = !input.textureQuadrants.empty();
    if (requirements.textureQuadrants == LandPayloadRequirement::Required && !hasTextureQuadrants) {
        throw std::invalid_argument("LAND assembly requires at least one texture quadrant.");
    }
    if (requirements.textureQuadrants == LandPayloadRequirement::Forbidden && hasTextureQuadrants) {
        throw std::invalid_argument("LAND assembly forbids texture quadrants.");
    }

    for (const LandTextureQuadrantPayloadGroup& textureGroup : input.textureQuadrants) {
        if (textureGroup.btxtPayload.has_value() != textureGroup.baseLayerIdentity.has_value()) {
            throw std::invalid_argument(
                "LAND texture quadrant must provide BTXT payload and base-layer identity together.");
        }
        if (textureGroup.baseLayerIdentity.has_value() &&
            textureGroup.baseLayerIdentity->encodedQuadrantValue !=
                textureGroup.encodedQuadrantValue) {
            throw std::invalid_argument(
                "LAND BTXT base-layer identity targets a different quadrant than its texture group.");
        }
        ValidateRequirement(textureGroup.btxtPayload, requirements.btxtPerTextureQuadrant, "BTXT");

        for (const LandAlphaTexturePayloadGroup& alphaLayer : textureGroup.alphaLayers) {
            if (alphaLayer.identity.encodedQuadrantValue !=
                textureGroup.encodedQuadrantValue) {
                throw std::invalid_argument(
                    "LAND alpha layer targets a different quadrant than its texture group.");
            }
            ValidateRequirement(alphaLayer.atxtPayload, requirements.atxtPerAlphaLayer, "ATXT");
            ValidateRequirement(alphaLayer.vtxtPayload, requirements.vtxtPerAlphaLayer, "VTXT");
        }
    }

    std::vector<AssembledLandSubrecord> output;
    for (const LandTopLevelOrderItem orderItem : *rules_.topLevelOrder) {
        switch (orderItem) {
        case LandTopLevelOrderItem::Data:
            AppendPayload(output, LandSubrecordType::Data, input.dataPayload);
            break;
        case LandTopLevelOrderItem::Vhgt:
            AppendPayload(output, LandSubrecordType::Vhgt, input.vhgtPayload);
            break;
        case LandTopLevelOrderItem::Vnml:
            AppendPayload(output, LandSubrecordType::Vnml, input.vnmlPayload);
            break;
        case LandTopLevelOrderItem::Vclr:
            AppendPayload(output, LandSubrecordType::Vclr, input.vclrPayload);
            break;
        case LandTopLevelOrderItem::TextureQuadrants:
            for (const LandTextureQuadrantPayloadGroup& textureGroup : input.textureQuadrants) {
                for (const LandTextureGroupOrderItem textureOrderItem :
                     *rules_.textureGroupOrder) {
                    switch (textureOrderItem) {
                    case LandTextureGroupOrderItem::Btxt:
                        AppendPayload(
                            output,
                            LandSubrecordType::Btxt,
                            textureGroup.btxtPayload,
                            textureGroup.baseLayerIdentity);
                        break;
                    case LandTextureGroupOrderItem::AlphaLayers:
                        for (const LandAlphaTexturePayloadGroup& alphaLayer :
                             textureGroup.alphaLayers) {
                            for (const LandAlphaLayerOrderItem alphaOrderItem :
                                 *rules_.alphaLayerOrder) {
                                switch (alphaOrderItem) {
                                case LandAlphaLayerOrderItem::Atxt:
                                    AppendPayload(
                                        output,
                                        LandSubrecordType::Atxt,
                                        alphaLayer.atxtPayload,
                                        alphaLayer.identity);
                                    break;
                                case LandAlphaLayerOrderItem::Vtxt:
                                    AppendPayload(
                                        output,
                                        LandSubrecordType::Vtxt,
                                        alphaLayer.vtxtPayload,
                                        alphaLayer.identity);
                                    break;
                                }
                            }
                        }
                        break;
                    }
                }
            }
            break;
        }
    }

    return output;
}

void LandSubrecordAssembler::WriteSubrecords(
    const LandSubrecordAssemblyInput& input,
    RecordWriter& record) const {
    const std::vector<AssembledLandSubrecord> subrecords = Assemble(input);
    for (const AssembledLandSubrecord& subrecord : subrecords) {
        SubRecordWriter writer(record, ToSignature(subrecord.type));
        if (!subrecord.payload.empty()) {
            writer.WriteBytes(subrecord.payload.data(), subrecord.payload.size());
        }
        writer.Finalize();
    }
}

void LandSubrecordAssembler::ValidateRules() const {
    if (!rules_.topLevelOrder.has_value() || !rules_.textureGroupOrder.has_value() ||
        !rules_.alphaLayerOrder.has_value() || !rules_.presenceRequirements.has_value()) {
        throw MissingSpecification(
            "LAND subrecord assembly requires a complete, confirmed Skyrim SE/AE ordering specification.");
    }

    ValidatePermutation<LandTopLevelOrderItem, kTopLevelItemCount>(
        *rules_.topLevelOrder, "top-level order");
    ValidatePermutation<LandTextureGroupOrderItem, kTextureGroupItemCount>(
        *rules_.textureGroupOrder, "texture-group order");
    ValidatePermutation<LandAlphaLayerOrderItem, kAlphaLayerItemCount>(
        *rules_.alphaLayerOrder, "alpha-layer order");
}

} // namespace land_ir
