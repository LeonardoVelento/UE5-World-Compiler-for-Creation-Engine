#include "UEWorldReader.h"

#include "Components/LightComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Light.h"
#include "Engine/PointLight.h"
#include "Engine/RectLight.h"
#include "Engine/SpotLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Landscape.h"
#include "LandscapeDataAccess.h"
#include "LandscapeEdit.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeProxy.h"
#include "LandscapeStreamingProxy.h"
#include "UObject/Class.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

std::string ToStdString(const FString& value) {
    return TCHAR_TO_UTF8(*value);
}

world_ir::Vector3 ToVector3(const FVector& value) {
    return {value.X, value.Y, value.Z};
}

world_ir::Rotation ToRotation(const FRotator& value) {
    return {value.Pitch, value.Yaw, value.Roll};
}

world_ir::Color ToColor(const FLinearColor& value) {
    return {value.R, value.G, value.B, value.A};
}

world_ir::AssetID ToAssetID(const UObject* asset) {
    return asset == nullptr ? world_ir::AssetID{} : ToStdString(asset->GetName());
}

world_ir::PersistentID ReadPersistentID(const AActor& actor) {
#if WITH_EDITOR
    const FGuid& actorGuid = actor.GetActorGuid();
    if (actorGuid.IsValid()) {
        return ToStdString(actorGuid.ToString());
    }
#endif

    return ToStdString(actor.GetPathName());
}

world_ir::Metadata ReadMetadata(const AActor& actor) {
    world_ir::Metadata metadata;
    metadata.emplace("unreal.actor_path", ToStdString(actor.GetPathName()));

#if WITH_EDITOR
    metadata.emplace("unreal.actor_label", ToStdString(actor.GetActorLabel()));
#endif

    for (int32 index = 0; index < actor.Tags.Num(); ++index) {
        metadata.emplace("unreal.tag." + std::to_string(index), ToStdString(actor.Tags[index].ToString()));
    }

    return metadata;
}

bool IsBlueprintActor(const AActor& actor) {
    return actor.GetClass()->ClassGeneratedBy != nullptr;
}

bool IsSupportedLight(const ALight& actor) {
    return actor.IsA<APointLight>() || actor.IsA<ASpotLight>() || actor.IsA<ARectLight>() ||
           actor.IsA<ADirectionalLight>();
}

world_ir::Object ReadStaticMeshObject(const AStaticMeshActor& actor) {
    world_ir::Object object;
    object.persistentID = ReadPersistentID(actor);

    const UStaticMeshComponent* meshComponent = actor.GetStaticMeshComponent();
    object.assetID = meshComponent == nullptr ? world_ir::AssetID{} : ToAssetID(meshComponent->GetStaticMesh());
    object.position = ToVector3(actor.GetActorLocation());
    object.rotation = ToRotation(actor.GetActorRotation());
    object.scale = ToVector3(actor.GetActorScale3D());
    object.metadata = ReadMetadata(actor);
    return object;
}

world_ir::Light ReadLight(const ALight& actor) {
    world_ir::Light light;
    light.persistentID = ReadPersistentID(actor);
    light.assetID = ToAssetID(actor.GetClass());
    light.position = ToVector3(actor.GetActorLocation());
    light.rotation = ToRotation(actor.GetActorRotation());
    light.color = ToColor(actor.GetLightColor());
    light.metadata = ReadMetadata(actor);

    const ULightComponent* lightComponent = actor.GetLightComponent();
    if (lightComponent != nullptr) {
        light.intensity = lightComponent->Intensity;

        if (const ULocalLightComponent* localLight = Cast<ULocalLightComponent>(lightComponent)) {
            light.radius = localLight->AttenuationRadius;
        }
    }

    return light;
}

world_ir::Landscape ReadLandscape(const ALandscapeProxy& landscape) {
    world_ir::Landscape result;
    result.transform.position = ToVector3(landscape.GetActorLocation());
    result.transform.rotation = ToRotation(landscape.GetActorRotation());
    result.transform.scale = ToVector3(landscape.GetActorScale3D());
    result.baseLayer.assetID = ToAssetID(landscape.GetLandscapeMaterial(0));

    ULandscapeInfo* landscapeInfo = landscape.GetLandscapeInfo();
    if (landscapeInfo == nullptr) {
        return result;
    }

    // World Partition's root actor can have no local components even while
    // streaming proxies own the actual heightmap. LandscapeInfo exposes the
    // authoritative extent of loaded landscape components.
    FIntRect bounds;
    if (!landscapeInfo->GetLandscapeExtent(bounds) ||
        bounds.Max.X < bounds.Min.X || bounds.Max.Y < bounds.Min.Y) {
        return result;
    }

    const int32 width = bounds.Max.X - bounds.Min.X + 1;
    const int32 height = bounds.Max.Y - bounds.Min.Y + 1;
    result.heightmap.width = static_cast<std::uint32_t>(width);
    result.heightmap.height = static_cast<std::uint32_t>(height);
    result.heightmap.firstLocalSampleX = bounds.Min.X;
    result.heightmap.firstLocalSampleY = bounds.Min.Y;

    std::vector<std::uint16_t> encodedHeights(
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height));

    FLandscapeEditDataInterface editData(landscapeInfo, false);
    editData.GetHeightDataFast(bounds.Min.X,
                               bounds.Min.Y,
                               bounds.Max.X,
                               bounds.Max.Y,
                               encodedHeights.data(),
                               width,
                               nullptr,
                               nullptr);

    result.heightmap.localHeights.resize(encodedHeights.size());
    for (std::size_t index = 0; index < encodedHeights.size(); ++index) {
        result.heightmap.localHeights[index] =
            static_cast<double>(LandscapeDataAccess::GetLocalHeight(encodedHeights[index]));
    }

    for (const TPair<FName, FLandscapeTargetLayerSettings>& entry : landscape.GetTargetLayers()) {
        ULandscapeLayerInfoObject* layerInfo = entry.Value.LayerInfoObj;
        if (layerInfo == nullptr) {
            continue;
        }

        world_ir::LandscapeLayer layer;
        layer.assetID = ToAssetID(layerInfo);
        layer.blendMap.width = static_cast<std::uint32_t>(width);
        layer.blendMap.height = static_cast<std::uint32_t>(height);
        layer.blendMap.weights.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));

        editData.GetWeightDataFast(layerInfo,
                                   bounds.Min.X,
                                   bounds.Min.Y,
                                   bounds.Max.X,
                                   bounds.Max.Y,
                                   layer.blendMap.weights.data(),
                                   width);

        result.layers.push_back(std::move(layer));
    }

    return result;
}

} // namespace

world_ir::World UEWorldReader::Read(UWorld* world) {
    if (world == nullptr) {
        throw std::invalid_argument("UEWorldReader::Read requires a valid UWorld.");
    }

    world_ir::World result;
    result.name = ToStdString(world->GetName());
    result.metadata.emplace("unreal.world_path", ToStdString(world->GetPathName()));

    // Prefer a complete main Landscape, but also support World Partition's
    // LandscapeStreamingProxy actors. Empty Landscape actors are ignored.
    const auto tryReadLandscape = [&result](const ALandscapeProxy* landscape) {
        if (landscape == nullptr) {
            return false;
        }

        world_ir::Landscape candidate = ReadLandscape(*landscape);
        if (candidate.heightmap.width < 2 || candidate.heightmap.height < 2 ||
            candidate.heightmap.localHeights.empty()) {
            return false;
        }

        result.landscape = std::move(candidate);
        return true;
    };

    for (TActorIterator<ALandscape> landscapeIt(world); landscapeIt; ++landscapeIt) {
        if (tryReadLandscape(*landscapeIt)) {
            break;
        }
    }

    if (result.landscape.heightmap.width < 2 || result.landscape.heightmap.height < 2) {
        for (TActorIterator<ALandscapeProxy> landscapeIt(world); landscapeIt; ++landscapeIt) {
            const ALandscapeProxy* landscape = *landscapeIt;
            if (landscape != nullptr && !landscape->IsA<ALandscape>() &&
                tryReadLandscape(landscape)) {
                break;
            }
        }
    }

    for (TActorIterator<AStaticMeshActor> actorIt(world); actorIt; ++actorIt) {
        const AStaticMeshActor* actor = *actorIt;
        if (actor == nullptr || IsBlueprintActor(*actor)) {
            continue;
        }

        result.objects.push_back(ReadStaticMeshObject(*actor));
    }

    for (TActorIterator<ALight> lightIt(world); lightIt; ++lightIt) {
        const ALight* light = *lightIt;
        if (light == nullptr || !IsSupportedLight(*light)) {
            continue;
        }

        result.lights.push_back(ReadLight(*light));
    }

    return result;
}
