#include "CreationCoordinateTransform.h"

world_ir::Vector3 ToCreationPosition(const world_ir::Vector3& unrealPosition,
                                     const CreationTransformSettings& settings) {
    world_ir::Vector3 creationPosition{
        (unrealPosition.x - settings.worldOriginOffset.x) * settings.unitScale,
        (unrealPosition.y - settings.worldOriginOffset.y) * settings.unitScale,
        (unrealPosition.z - settings.worldOriginOffset.z) * settings.unitScale,
    };

    if (settings.invertYAxis) {
        creationPosition.y = -creationPosition.y;
    }

    return creationPosition;
}
