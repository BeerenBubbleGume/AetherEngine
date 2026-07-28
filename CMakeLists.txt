cmake_minimum_required(VERSION 3.25)
project(smb)

set(CMAKE_CXX_STANDARD 23)

find_package(SDL3 CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)


if (MSVC)
    add_compile_options("/Zc:__cplusplus" "/Zc:preprocessor")
endif()
if (MSVC)
    add_compile_options("/permissive-")
endif()

add_library(engine STATIC
        engine/src/window/Window.cpp
        engine/src/core/Engine.cpp
        engine/headers/core/Engine.hpp
        engine/src/systems/InputSystem.cpp
        engine/headers/systems/InputSystem.hpp
        engine/headers/interfaces/IUpdatable.hpp
        engine/src/systems/RenderSystem.cpp
        engine/headers/systems/RenderSystem.hpp
        engine/src/graphics/Mesh.cpp
        engine/headers/graphics/Mesh.hpp
        engine/src/graphics/ShaderProgram.cpp
        engine/headers/graphics/ShaderProgram.hpp
        engine/src/resources/ResourceManager.cpp
        engine/headers/resources/ResourceManager.hpp
        engine/headers/resources/EmbeddedShaders.hpp
        engine/src/scene/Scene.cpp
        engine/headers/scene/Scene.hpp
        engine/src/scene/Camera.cpp
        engine/headers/scene/Camera.hpp
        engine/headers/core/IApplication.hpp
        engine/src/core/EngineContext.cpp
        engine/headers/core/EngineContext.hpp
        engine/headers/components/CameraComponent.hpp
        engine/headers/components/MeshComponent.hpp
        engine/headers/components/TransformComponent.hpp
        engine/headers/components/IdentityComponent.hpp
        engine/headers/scene/Entity.hpp
        engine/src/math/Types.cpp
        engine/headers/math/Types.hpp
        engine/headers/math/Detail.hpp
        engine/src/math/Detail.cpp
        engine/headers/components/MaterialComponent.hpp
        engine/headers/math/CameraMatrices.hpp
        engine/src/math/CameraMatrices.cpp
        engine/headers/systems/InputTypes.hpp
        engine/headers/resources/ResourceTypes.hpp
        engine/src/graphics/Texture.cpp
        engine/headers/graphics/Texture.hpp
        engine/src/graphics/RenderTarget.cpp
        engine/headers/graphics/RenderTarget.hpp
        engine/headers/graphics/SceneView.hpp
        engine/src/systems/SceneSerializer.cpp
        engine/headers/systems/SceneSerializer.hpp
        engine/headers/components/PhysicsComponents.hpp
        engine/src/systems/PhysicsSystem.cpp
        engine/headers/systems/PhysicsSystem.hpp
        engine/src/security/PathSecurity.cpp
        engine/headers/security/PathSecurity.hpp
        engine/headers/security/AssetValidation.hpp
        engine/headers/physics/PhysicsTypes.hpp
        engine/src/physics/IPhysicsBackend.hpp
        engine/src/physics/PhysicsBackendFactory.cpp
        engine/src/physics/PhysicsBackendFactory.hpp
        engine/src/physics/physx/PhysXBackend.cpp
        engine/src/physics/physx/PhysXBackend.hpp
)

target_include_directories(engine
        PUBLIC engine/headers
        PRIVATE engine/src
)

if (MSVC)
    target_compile_options(engine PRIVATE "/Zc:__cplusplus" "/Zc:preprocessor")
endif()

include(CTest)
if (BUILD_TESTING)
    add_executable(aether_security_tests
            tests/SecurityRegressionTests.cpp
    )
    target_link_libraries(aether_security_tests PRIVATE engine)
    target_compile_definitions(aether_security_tests PRIVATE
            AETHER_TEST_ASSETS_ROOT="${CMAKE_SOURCE_DIR}/assets"
    )
    add_test(NAME security_regressions COMMAND aether_security_tests)
endif()

add_executable(smb
        smb/src/core/Smb.cpp
        smb/headers/core/Smb.hpp
        smb/src/main.cpp
)

add_executable(AetherEditor
        editor/src/main.cpp
        editor/src/core/AetherEditor.cpp
        editor/headers/core/AetherEditor.hpp
        editor/src/ui/EditorGui.cpp
        editor/headers/ui/EditorGui.hpp
        editor/src/ui/ImGuiBgfxRenderer.cpp
        editor/headers/ui/ImGuiBgfxRenderer.hpp
        editor/headers/core/ETypes.hpp
)

set(AETHER_IMGUI_SHADER_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/assets/shaders/generated")
set(AETHER_IMGUI_SHADER_HEADERS
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}/imgui_fs_dx11.bin.h"
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}/imgui_fs_essl.bin.h"
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}/imgui_fs_glsl.bin.h"
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}/imgui_fs_mtl.bin.h"
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}/imgui_fs_spv.bin.h"
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}/imgui_vs_dx11.bin.h"
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}/imgui_vs_essl.bin.h"
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}/imgui_vs_glsl.bin.h"
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}/imgui_vs_mtl.bin.h"
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}/imgui_vs_spv.bin.h"
)

target_sources(AetherEditor PRIVATE ${AETHER_IMGUI_SHADER_HEADERS})

target_include_directories(smb PRIVATE smb/headers)
target_include_directories(AetherEditor PRIVATE
        editor/headers
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}"
)

set(ENGINE_ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")

function(link_engine_assets target_name)
    set(runtime_assets_dir "$<TARGET_FILE_DIR:${target_name}>/assets")
    add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf
            "${runtime_assets_dir}"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ENGINE_ASSETS_SOURCE_DIR}"
            "${runtime_assets_dir}"
            COMMENT "Copying ${target_name} runtime assets"
            VERBATIM
    )
endfunction()

link_engine_assets(smb)
link_engine_assets(AetherEditor)

target_link_libraries(engine PUBLIC
        SDL3::SDL3
        EnTT::EnTT
        bgfx::bgfx
        glm::glm
        nlohmann_json::nlohmann_json
)

target_link_libraries(engine PRIVATE
        unofficial::omniverse-physx-sdk::sdk
)

target_link_libraries(smb PRIVATE
        engine
)

target_link_libraries(AetherEditor PRIVATE
        engine
        imgui::imgui
)

if (MSVC)
    set(hardened_targets engine smb AetherEditor)
    set(hardened_executables smb AetherEditor)
    if (TARGET aether_security_tests)
        list(APPEND hardened_targets aether_security_tests)
        list(APPEND hardened_executables aether_security_tests)
    endif()
    foreach(hardened_target IN LISTS hardened_targets)
        target_compile_options(${hardened_target} PRIVATE "/sdl" "/guard:cf")
    endforeach()
    foreach(hardened_executable IN LISTS hardened_executables)
        target_link_options(${hardened_executable} PRIVATE
                "/guard:cf"
                "/DYNAMICBASE"
                "/NXCOMPAT"
                "/DEPENDENTLOADFLAG:0xA00"
        )
    endforeach()
endif()
