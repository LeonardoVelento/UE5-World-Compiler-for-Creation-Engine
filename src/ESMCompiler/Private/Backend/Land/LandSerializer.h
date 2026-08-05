#pragma once

#include "BinaryWriter.h"
#include "ExperimentalLandDataEncoder.h"
#include "LandRecord.h"
#include "LandSubrecordAssembler.h"
#include "LandSubrecordEncoder.h"
#include "MissingSpecification.h"

struct LandEncoderSet {
    const land_ir::ILandDataEncoder* data = nullptr;
    const land_ir::ILandVhgtEncoder* vhgt = nullptr;
    const land_ir::ILandVnmlEncoder* vnml = nullptr;
    const land_ir::ILandVclrEncoder* vclr = nullptr;
};

// Texture payloads are not represented in LandEncoderSet: one LAND owns
// several BTXT/ATXT/VTXT payloads. SkyrimLandTexturePayloadPlanner derives
// those groups directly from the completed LandTile.
class LandSerializer final {
public:
    explicit LandSerializer(LandEncoderSet encoders = {});

    // An opt-in research serializer. It emits all current LAND payloads and
    // uses ExperimentalLandDataEncoder for DATA. Its 0x08 and 0x10 policy is
    // intentionally not a claim that CK's internal generation is confirmed.
    [[nodiscard]] static LandSerializer MakeExperimentalSkyrimSEAE();

    // Performs the same preflight as Serialize without touching BinaryWriter.
    // Group serializers use it to avoid partial plugin output.
    void EnsureSerializable(const LandRecord& record) const;

    void Serialize(const LandRecord& record, BinaryWriter& writer) const;

private:
    [[nodiscard]] land_ir::LandSubrecordAssemblyInput BuildAssemblyInput(
        const LandRecord& record) const;

    LandEncoderSet encoders_;
};
