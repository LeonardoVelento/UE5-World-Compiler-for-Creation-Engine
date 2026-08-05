#pragma once

#include "LandSubrecordEncoder.h"
#include "RecordWriter.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace land_ir {

struct LandTexturePayloadIdentity {
    std::size_t sourceLayerIndex = 0;
    std::uint32_t encodedQuadrantValue = 0;

    bool operator==(const LandTexturePayloadIdentity&) const = default;
};

// ATXT and VTXT are deliberately stored in the same object so the assembler
// can preserve their association without attempting to reconstruct it.
struct LandAlphaTexturePayloadGroup {
    LandTexturePayloadIdentity identity;
    std::optional<std::vector<std::uint8_t>> atxtPayload;
    std::optional<std::vector<std::uint8_t>> vtxtPayload;

    bool operator==(const LandAlphaTexturePayloadGroup&) const = default;
};

struct LandTextureQuadrantPayloadGroup {
    // This identifies the quadrant even when CK has no BTXT for it.
    std::uint32_t encodedQuadrantValue = 0;

    // BTXT is optional per quadrant. Its identity is meaningful only when
    // btxtPayload is present.
    std::optional<LandTexturePayloadIdentity> baseLayerIdentity;
    std::optional<std::vector<std::uint8_t>> btxtPayload;
    std::vector<LandAlphaTexturePayloadGroup> alphaLayers;

    bool operator==(const LandTextureQuadrantPayloadGroup&) const = default;
};

struct LandSubrecordAssemblyInput {
    std::optional<std::vector<std::uint8_t>> dataPayload;
    std::optional<std::vector<std::uint8_t>> vhgtPayload;
    std::optional<std::vector<std::uint8_t>> vnmlPayload;
    std::optional<std::vector<std::uint8_t>> vclrPayload;

    // Input order is retained for groups and alpha layers unless future,
    // confirmed Skyrim rules explicitly introduce a different planner.
    std::vector<LandTextureQuadrantPayloadGroup> textureQuadrants;
};

enum class LandTopLevelOrderItem {
    Data,
    Vhgt,
    Vnml,
    Vclr,
    TextureQuadrants,
};

enum class LandTextureGroupOrderItem {
    Btxt,
    AlphaLayers,
};

enum class LandAlphaLayerOrderItem {
    Atxt,
    Vtxt,
};

enum class LandPayloadRequirement {
    Required,
    Optional,
    Forbidden,
};

struct LandSubrecordPresenceRequirements {
    LandPayloadRequirement data = LandPayloadRequirement::Forbidden;
    LandPayloadRequirement vhgt = LandPayloadRequirement::Forbidden;
    LandPayloadRequirement vnml = LandPayloadRequirement::Forbidden;
    LandPayloadRequirement vclr = LandPayloadRequirement::Forbidden;
    LandPayloadRequirement textureQuadrants = LandPayloadRequirement::Forbidden;
    LandPayloadRequirement btxtPerTextureQuadrant = LandPayloadRequirement::Forbidden;
    LandPayloadRequirement atxtPerAlphaLayer = LandPayloadRequirement::Forbidden;
    LandPayloadRequirement vtxtPerAlphaLayer = LandPayloadRequirement::Forbidden;
};

// All order and presence behavior is supplied explicitly. Empty payloads are
// distinct from missing payloads because presence is represented by optional.
struct LandSubrecordAssemblyRules {
    std::optional<std::vector<LandTopLevelOrderItem>> topLevelOrder;
    std::optional<std::vector<LandTextureGroupOrderItem>> textureGroupOrder;
    std::optional<std::vector<LandAlphaLayerOrderItem>> alphaLayerOrder;
    std::optional<LandSubrecordPresenceRequirements> presenceRequirements;
};

// The file-order grammar observed for Skyrim SE/AE LAND records:
// DATA, VNML, VHGT, VCLR, followed by texture layer groups where every ATXT
// is immediately followed by its VTXT. This function contains only that
// assembly grammar; it does not assign texture quadrants or layer indices.
[[nodiscard]] LandSubrecordAssemblyRules MakeSkyrimSEAELandSubrecordAssemblyRules();

struct AssembledLandSubrecord {
    LandSubrecordType type = LandSubrecordType::Data;
    std::vector<std::uint8_t> payload;
    std::optional<LandTexturePayloadIdentity> textureIdentity;

    bool operator==(const AssembledLandSubrecord&) const = default;
};

// This class contains no binary LAND format knowledge. It expands only a
// supplied order grammar, retaining project input order inside its groups.
class LandSubrecordAssembler final {
public:
    explicit LandSubrecordAssembler(LandSubrecordAssemblyRules rules = {});

    [[nodiscard]] std::vector<AssembledLandSubrecord> Assemble(
        const LandSubrecordAssemblyInput& input) const;

    // Writes only after Assemble has fully completed, so a validation failure
    // cannot leave a partially written record payload.
    void WriteSubrecords(const LandSubrecordAssemblyInput& input, RecordWriter& record) const;

private:
    void ValidateRules() const;

    LandSubrecordAssemblyRules rules_;
};

} // namespace land_ir
