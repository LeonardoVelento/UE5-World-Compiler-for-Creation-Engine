#pragma once

#include "LandscapeTextureQuadrantAssigner.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace land_ir {

// Identifies the one logical ATXT assignment to which a VTXT entry belongs.
// The combination is supplied by the quadrant-assignment stage and does not
// represent an on-disk record or subrecord position.
struct AtxtLayerIdentity {
    std::size_t sourceLayerIndex = 0;
    std::uint32_t encodedQuadrantValue = 0;

    bool operator==(const AtxtLayerIdentity&) const = default;
};

struct VtxtEntry {
    AtxtLayerIdentity atxtLayer;
    std::size_t vertexPosition = 0;
    float opacity = 0.0F;

    bool operator==(const VtxtEntry&) const = default;
};

// The resolver supplies the confirmed Skyrim VTXT vertex position/index for
// one local quadrant sample. It is intentionally separate from the builder so
// this module never invents Skyrim vertex indexing.
class IVtxtVertexIndexResolver {
public:
    virtual ~IVtxtVertexIndexResolver() = default;

    [[nodiscard]] virtual std::optional<std::size_t> ResolveVertexPosition(
        const QuadrantLocalTextureLayerInput& atxtLayer,
        const QuadrantLocalLayerWeight& sample) const = 0;
};

enum class VtxtZeroOpacityHandling {
    Preserve,
    Omit,
};

// An explicit policy is required because zero-opacity omission is format
// behavior, not an optimization the compiler is allowed to assume.
struct VtxtEntryBuilderRules {
    std::optional<VtxtZeroOpacityHandling> zeroOpacityHandling;
};

// VTXT uses the already-assigned 17x17 quadrant-local index directly. The
// assigner is responsible for proving that it lies in the confirmed 0..288
// range; this resolver repeats that validation at the interface boundary.
class SkyrimSEAEVtxtVertexIndexResolver final : public IVtxtVertexIndexResolver {
public:
    [[nodiscard]] std::optional<std::size_t> ResolveVertexPosition(
        const QuadrantLocalTextureLayerInput& atxtLayer,
        const QuadrantLocalLayerWeight& sample) const override;
};

// The current AE MVP omits zero-opacity entries. The texture planner omits
// the whole ATXT/VTXT pair when this leaves an additional layer with no
// entries in a quadrant.
[[nodiscard]] VtxtEntryBuilderRules MakeSkyrimSEAEVtxtEntryBuilderRules();

// Builds non-binary VTXT IR for a single, resolved ATXT layer assignment.
// Input sample order is preserved, making output deterministic for identical
// input and resolver results.
class VtxtEntryBuilder final {
public:
    explicit VtxtEntryBuilder(
        const IVtxtVertexIndexResolver* vertexIndexResolver = nullptr,
        VtxtEntryBuilderRules rules = {});

    [[nodiscard]] std::vector<VtxtEntry> Build(
        const QuadrantLocalTextureLayerInput& atxtLayer) const;

private:
    void ValidateDependencies() const;

    const IVtxtVertexIndexResolver* vertexIndexResolver_ = nullptr;
    VtxtEntryBuilderRules rules_;
};

} // namespace land_ir
