#include "LandSerializer.h"

#include "LandSubrecordAssembler.h"
#include "LandTexturePayloadPlanner.h"
#include "SkyrimVhgtSubrecordEncoder.h"
#include "SubRecordWriter.h"
#include "VclrEncoder.h"
#include "VnmlEncoder.h"

#include <array>
#include <stdexcept>

namespace {

template <typename Encoder, typename Placeholder>
const Encoder& ResolveEncoder(const Encoder* suppliedEncoder, const Placeholder& placeholder) {
    return suppliedEncoder != nullptr ? *suppliedEncoder : placeholder;
}

const std::vector<std::uint8_t>& RequirePayloadType(
    const land_ir::EncodedLandSubrecord& encoded,
    land_ir::LandSubrecordType expectedType) {
    const std::array<char, 4> expectedSignature = [&] {
        switch (expectedType) {
        case land_ir::LandSubrecordType::Data:
            return std::array<char, 4>{'D', 'A', 'T', 'A'};
        case land_ir::LandSubrecordType::Vhgt:
            return std::array<char, 4>{'V', 'H', 'G', 'T'};
        case land_ir::LandSubrecordType::Vnml:
            return std::array<char, 4>{'V', 'N', 'M', 'L'};
        case land_ir::LandSubrecordType::Vclr:
            return std::array<char, 4>{'V', 'C', 'L', 'R'};
        default:
            throw std::logic_error("LAND serializer requested an unsupported core subrecord type.");
        }
    }();
    if (encoded.type != expectedSignature) {
        throw std::invalid_argument("LAND encoder returned a payload with the wrong subrecord signature.");
    }
    return encoded.payload;
}

void ValidateLandOwnership(const LandRecord& record) {
    if (record.formID == 0) {
        throw std::invalid_argument("LAND serialization requires a non-zero LAND FormID.");
    }
    if (record.owningCellFormID == 0) {
        throw std::invalid_argument("LAND serialization requires an owning exterior CELL FormID.");
    }
}

} // namespace

LandSerializer::LandSerializer(LandEncoderSet encoders)
    : encoders_(encoders) {}

LandSerializer LandSerializer::MakeExperimentalSkyrimSEAE() {
    static const land_ir::ExperimentalLandDataEncoder dataEncoder;
    static const land_ir::SkyrimVhgtSubrecordEncoder vhgtEncoder;
    static const land_ir::VnmlEncoder vnmlEncoder;
    static const land_ir::VclrEncoder vclrEncoder;
    return LandSerializer({
        .data = &dataEncoder,
        .vhgt = &vhgtEncoder,
        .vnml = &vnmlEncoder,
        .vclr = &vclrEncoder,
    });
}

land_ir::LandSubrecordAssemblyInput LandSerializer::BuildAssemblyInput(
    const LandRecord& record) const {
    ValidateLandOwnership(record);

    if (!record.landTile.has_value()) {
        throw std::invalid_argument("LAND serialization requires a completed LandTile.");
    }

    static const land_ir::MissingDataEncoder missingDataEncoder;
    static const land_ir::SkyrimVhgtSubrecordEncoder defaultVhgtEncoder;
    static const land_ir::VnmlEncoder defaultVnmlEncoder;
    static const land_ir::VclrEncoder defaultVclrEncoder;

    const land_ir::LandTile& tile = *record.landTile;
    const land_ir::EncodedLandSubrecord data =
        ResolveEncoder(encoders_.data, missingDataEncoder).Encode(tile);
    const land_ir::EncodedLandSubrecord vhgt =
        ResolveEncoder(encoders_.vhgt, defaultVhgtEncoder).Encode(tile);
    const land_ir::EncodedLandSubrecord vnml =
        ResolveEncoder(encoders_.vnml, defaultVnmlEncoder).Encode(tile);
    const land_ir::EncodedLandSubrecord vclr =
        ResolveEncoder(encoders_.vclr, defaultVclrEncoder).Encode(tile);

    const land_ir::SkyrimLandTexturePayloadPlanner texturePlanner;
    return {
        .dataPayload = RequirePayloadType(data, land_ir::LandSubrecordType::Data),
        .vhgtPayload = RequirePayloadType(vhgt, land_ir::LandSubrecordType::Vhgt),
        .vnmlPayload = RequirePayloadType(vnml, land_ir::LandSubrecordType::Vnml),
        .vclrPayload = RequirePayloadType(vclr, land_ir::LandSubrecordType::Vclr),
        .textureQuadrants = texturePlanner.Plan(tile),
    };
}

void LandSerializer::EnsureSerializable(const LandRecord& record) const {
    const land_ir::LandSubrecordAssemblyInput input = BuildAssemblyInput(record);
    const land_ir::LandSubrecordAssembler assembler(
        land_ir::MakeSkyrimSEAELandSubrecordAssemblyRules());
    static_cast<void>(assembler.Assemble(input));
}

void LandSerializer::Serialize(const LandRecord& record, BinaryWriter& writer) const {
    // Complete every encoder and the assembly before RecordWriter construction.
    // A specification or validation error therefore cannot leave a partial LAND
    // header in the plugin file.
    const land_ir::LandSubrecordAssemblyInput input = BuildAssemblyInput(record);
    const land_ir::LandSubrecordAssembler assembler(
        land_ir::MakeSkyrimSEAELandSubrecordAssemblyRules());
    const std::vector<land_ir::AssembledLandSubrecord> subrecords = assembler.Assemble(input);

    RecordWriter landRecord(writer, {
        .recordType = {'L', 'A', 'N', 'D'},
        .formID = record.formID,
    });
    for (const land_ir::AssembledLandSubrecord& subrecord : subrecords) {
        const std::array<char, 4> signature = [&] {
            switch (subrecord.type) {
            case land_ir::LandSubrecordType::Data:
                return std::array<char, 4>{'D', 'A', 'T', 'A'};
            case land_ir::LandSubrecordType::Vhgt:
                return std::array<char, 4>{'V', 'H', 'G', 'T'};
            case land_ir::LandSubrecordType::Vnml:
                return std::array<char, 4>{'V', 'N', 'M', 'L'};
            case land_ir::LandSubrecordType::Vclr:
                return std::array<char, 4>{'V', 'C', 'L', 'R'};
            case land_ir::LandSubrecordType::Btxt:
                return std::array<char, 4>{'B', 'T', 'X', 'T'};
            case land_ir::LandSubrecordType::Atxt:
                return std::array<char, 4>{'A', 'T', 'X', 'T'};
            case land_ir::LandSubrecordType::Vtxt:
                return std::array<char, 4>{'V', 'T', 'X', 'T'};
            }
            throw std::logic_error("LAND assembly produced an unknown subrecord type.");
        }();

        SubRecordWriter subrecordWriter(landRecord, signature);
        if (!subrecord.payload.empty()) {
            subrecordWriter.WriteBytes(subrecord.payload.data(), subrecord.payload.size());
        }
        subrecordWriter.Finalize();
    }
    landRecord.Finalize();
}
