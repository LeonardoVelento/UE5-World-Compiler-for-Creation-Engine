#include "WRLDGenerator.h"

WRLDRecord WRLDGenerator::Generate(const world_ir::World& world) const {
    WRLDRecord record;
    record.editorID = world.name;
    record.fullName = world.displayName;
    record.parentWorld.reset();
    record.worldFlags = WorldFlags::Default;
    record.climate.reset();
    record.water.reset();
    return record;
}
