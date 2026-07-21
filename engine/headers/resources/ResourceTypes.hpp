//
// Created by drhaz on 29.06.2026.
//

#ifndef SMB_RESOURCETYPES_HPP
#define SMB_RESOURCETYPES_HPP
#include <cstdint>
#include <vector>

#include "math/Types.hpp"

namespace engine::resources {
    struct MeshHandle final {
        uint32_t id = 0;
        [[nodiscard]] bool isValid() const { return id != 0; }
        bool operator==(const MeshHandle& other) const { return id == other.id; }
    };
    struct ProgramHandle final {
        uint32_t id = 0;
        [[nodiscard]] bool isValid() const { return id != 0; }
        bool operator==(const ProgramHandle& other) const { return id == other.id; }
    };
    struct TextureHandle final {
        uint32_t id = 0;
        [[nodiscard]] bool isValid() const { return id != 0; }
        bool operator==(const TextureHandle& other) const { return id == other.id; }
    };
    struct RenderState final {
        bool writeRgb{true};
        bool writeAlpha{true};
        bool writeDepth{true};
        bool depthTest{true};
        bool cullBackFaces{true};
        bool alphaBlend{false};
        bool msaa{true};
    };
    struct Material final {
        ProgramHandle program;
        math::Color baseColor;
        std::vector<TextureHandle> textures;
        RenderState renderState;
    };
}

#endif //SMB_RESOURCETYPES_HPP
