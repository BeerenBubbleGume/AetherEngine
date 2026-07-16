//
// Created by drhaz on 24.06.2026.
//

#ifndef SMB_ICAMERACOMPONENT_HPP
#define SMB_ICAMERACOMPONENT_HPP


namespace engine::components {
    enum class ProjectionType {
        Perspective = 0,
        Orthographic = 1
    };
    enum CameraClearFlags {
        ClearNone = 0,
        ClearColor = 1 << 0,
        ClearDepth = 1 << 1,
        ClearStencil = 1 << 2
    };
    struct ICameraComponent {
        bool enabled = true;

        ProjectionType projection{ProjectionType::Perspective};
        float fovYDegrees{60.0f};
        float nearPlane{0.1f};
        float farPlane{100.0f};

        float orthographicHeight{10.0f};

        uint8_t clearFlags{ClearColor | ClearDepth};
        uint32_t clearColor{0x303030ff};

        int priority{0};

        uint32_t layerMask{0xffffffff};
    };
}



#endif //SMB_ICAMERACOMPONENT_HPP
