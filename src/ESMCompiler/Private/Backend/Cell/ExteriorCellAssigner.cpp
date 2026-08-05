#include "ExteriorCellAssigner.h"

#include <cstddef>

CellGrid BuildCellGrid(const world_ir::World& world,
                       const CreationTransformSettings& transformSettings,
                       const CellCoordinateCalculator& calculator) {
    CellGrid grid;

    for (std::size_t objectIndex = 0; objectIndex < world.objects.size(); ++objectIndex) {
        const world_ir::Object& object = world.objects[objectIndex];
        const world_ir::Vector3 creationPosition =
            ToCreationPosition(object.position, transformSettings);
        const CellCoordinates coordinates =
            calculator.Calculate(creationPosition.x, creationPosition.y);

        auto [cellIterator, inserted] = grid.cells.try_emplace(
            coordinates,
            WorldCell{
                .coordinates = coordinates,
                .objectIndices = {},
                .containsLandscape = false,
            });
        (void)inserted;

        cellIterator->second.objectIndices.push_back(objectIndex);
    }

    return grid;
}
