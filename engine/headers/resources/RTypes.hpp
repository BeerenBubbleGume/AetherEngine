//
// Created by drhaz on 29.06.2026.
//

#ifndef SMB_RTYPES_HPP
#define SMB_RTYPES_HPP
#include <cstdint>
#include "math/UTypes.hpp"

namespace engine::resources {
    struct RMeshHandle final {
        uint32_t id = 0;
        [[nodiscard]] bool isValid() const { return id != 0; }
        bool operator==(const RMeshHandle& other) const { return id == other.id; }
    };
    struct RProgramHandle final {
        uint32_t id = 0;
        [[nodiscard]] bool isValid() const { return id != 0; }
        bool operator==(const RProgramHandle& other) const { return id == other.id; }
    };
    struct RTextureHandle final {
        uint32_t id = 0;
        [[nodiscard]] bool isValid() const { return id != 0; }
        bool operator==(const RTextureHandle& other) const { return id == other.id; }
    };
    struct RRenderState final {
        bool writeRgb{true};
        bool writeAlpha{true};
        bool writeDepth{true};
        bool depthTest{true};
        bool cullBackFaces{true};
        bool alphaBlend{false};
        bool msaa{true};
    };
    struct RMaterialHandle final {
        RProgramHandle program;
        math::TColor baseColor;
        std::vector<RTextureHandle> textures;
        RRenderState renderState;
    };
}

#endif //SMB_RTYPES_HPP
