#include "VertexColorGenerator.h"

#include <algorithm>

namespace land_ir {

VertexColorPatch VertexColorGenerator::Generate() const {
    VertexColorPatch patch;
    std::fill(patch.colors.begin(), patch.colors.end(), RGBVertexColor{255, 255, 255});
    return patch;
}

} // namespace land_ir
