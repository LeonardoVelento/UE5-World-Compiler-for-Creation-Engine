#pragma once

#include "LandIR.h"

namespace land_ir {

// MVP LAND vertex colours are authored by the compiler, not read from Unreal.
// Every generated vertex is opaque logical white RGB(255, 255, 255).
class VertexColorGenerator final {
public:
    [[nodiscard]] VertexColorPatch Generate() const;
};

} // namespace land_ir
