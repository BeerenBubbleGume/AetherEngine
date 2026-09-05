cmake_minimum_required(VERSION 3.25)
project(smb)

set(CMAKE_CXX_STANDARD 23)

find_package(SDL3 CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

if (APPLE)
    set(SMB_DEFAULT_PHYSICS_BACKEND "Jolt")
else()
    set(SMB_DEFAULT_PHYSICS_BACKEND "PhysX")
endif()
set(SMB_PHYSICS_BACKEND "${SMB_DEFAULT_PHYSICS_BACKEND}" CACHE STRING
        "Physics backend used by the engine (PhysX or Jolt)"
)
set_property(CACHE SMB_PHYSICS_BACKEND PROPERTY STRINGS PhysX Jolt)
string(TOLOWER "${SMB_PHYSICS_BACKEND}" SMB_PHYSICS_BACKEND_NORMALIZED)

if (SMB_PHYSICS_BACKEND_NORMALIZED STREQUAL "physx")
    find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)
    set(SMB_PHYSICS_BACKEND_SOURCES
            AetherEngine/src/physics/physx/PhysXBackend.cpp
            AetherEngine/src/physics/physx/PhysXBackend.hpp
    )
    set(SMB_PHYSICS_BACKEND_TARGET unofficial::omniverse-physx-sdk::sdk)
    set(SMB_PHYSICS_BACKEND_DEFINITION SMB_PHYSICS_BACKEND_PHYSX=1)
elseif (SMB_PHYSICS_BACKEND_NORMALIZED STREQUAL "jolt")
    find_package(Jolt CONFIG REQUIRED)
    set(SMB_PHYSICS_BACKEND_SOURCES
            AetherEngine/src/physics/jolt/JoltBackend.cpp
            AetherEngine/src/physics/jolt/JoltBackend.hpp
    )
    set(SMB_PHYSICS_BACKEND_TARGET Jolt::Jolt)
    set(SMB_PHYSICS_BACKEND_DEFINITION SMB_PHYSICS_BACKEND_JOLT=1)
else()
    message(FATAL_ERROR
            "Unsupported SMB_PHYSICS_BACKEND='${SMB_PHYSICS_BACKEND}'. "
            "Expected PhysX or Jolt."
    )
endif()


if (MSVC)
    add_compile_options("/Zc:__cplusplus" "/Zc:preprocessor")
endif()
if (MSVC)
    add_compile_options("/permissive-")
endif()

add_library(AetherEngine STATIC
        AetherEngine/src/window/Window.cpp
        AetherEngine/src/core/Engine.cpp
        AetherEngine/headers/core/AetherEngine.hpp
        AetherEngine/src/systems/InputSystem.cpp
        AetherEngine/headers/systems/InputSystem.hpp
        AetherEngine/headers/interfaces/IUpdatable.hpp
        AetherEngine/src/systems/RenderSystem.cpp
        AetherEngine/headers/systems/RenderSystem.hpp
        AetherEngine/src/graphics/Mesh.cpp
        AetherEngine/headers/graphics/Mesh.hpp
        AetherEngine/src/graphics/ShaderProgram.cpp
        AetherEngine/headers/graphics/ShaderProgram.hpp
        AetherEngine/src/resources/ResourceManager.cpp
        AetherEngine/headers/resources/ResourceManager.hpp
        AetherEngine/headers/resources/EmbeddedShaders.hpp
        AetherEngine/src/scene/Scene.cpp
        AetherEngine/headers/scene/Scene.hpp
        AetherEngine/src/scene/Camera.cpp
        AetherEngine/headers/scene/Camera.hpp
        AetherEngine/headers/core/IApplication.hpp
        AetherEngine/src/core/EngineContext.cpp
        AetherEngine/headers/core/EngineContext.hpp
        AetherEngine/headers/components/CameraComponent.hpp
        AetherEngine/headers/components/MeshComponent.hpp
        AetherEngine/headers/components/TransformComponent.hpp
        AetherEngine/headers/components/IdentityComponent.hpp
        AetherEngine/headers/scene/Entity.hpp
        AetherEngine/src/math/Types.cpp
        AetherEngine/headers/math/Types.hpp
        AetherEngine/headers/math/Detail.hpp
        AetherEngine/src/math/Detail.cpp
        AetherEngine/headers/components/MaterialComponent.hpp
        AetherEngine/headers/math/CameraMatrices.hpp
        AetherEngine/src/math/CameraMatrices.cpp
        AetherEngine/headers/systems/InputTypes.hpp
        AetherEngine/headers/resources/ResourceTypes.hpp
        AetherEngine/src/graphics/Texture.cpp
        AetherEngine/headers/graphics/Texture.hpp
        AetherEngine/src/graphics/RenderTarget.cpp
        AetherEngine/headers/graphics/RenderTarget.hpp
        AetherEngine/headers/graphics/SceneView.hpp
        AetherEngine/src/systems/SceneSerializer.cpp
        AetherEngine/headers/systems/SceneSerializer.hpp
        AetherEngine/headers/components/PhysicsComponents.hpp
        AetherEngine/src/systems/PhysicsSystem.cpp
        AetherEngine/headers/systems/PhysicsSystem.hpp
        AetherEngine/src/security/PathSecurity.cpp
        AetherEngine/headers/security/PathSecurity.hpp
        AetherEngine/headers/security/AssetValidation.hpp
        AetherEngine/headers/physics/PhysicsTypes.hpp
        AetherEngine/src/physics/IPhysicsBackend.hpp
        AetherEngine/src/physics/PhysicsBackendFactory.cpp
        AetherEngine/src/physics/PhysicsBackendFactory.hpp
        ${SMB_PHYSICS_BACKEND_SOURCES}
        AetherEngine/headers/assets/AssetId.hpp
        AetherEngine/headers/assets/AssetTypes.hpp
        AetherEngine/headers/assets/AssetMetadata.hpp
        AetherEngine/src/assets/AssetRegistry.cpp
        AetherEngine/headers/assets/AssetRegistry.hpp
        AetherEngine/src/assets/AssetManager.cpp
        AetherEngine/headers/assets/AssetManager.hpp
        AetherEngine/headers/assets/RuntimeAsset.hpp
        AetherEngine/headers/assets/IAssetLoader.hpp
        AetherEngine/headers/assets/AssetLoaders.hpp
        AetherEngine/headers/assets/PlatformProfile.hpp
        AetherEngine/src/assets/PlatformResolver.cpp
        AetherEngine/src/assets/loaders/MeshLoader.cpp
        AetherEngine/src/assets/loaders/TextureLoader.cpp
        AetherEngine/src/assets/loaders/ShaderLoader.cpp
        AetherEngine/src/assets/loaders/MaterialLoader.cpp
        AetherEngine/src/systems/SceneAssetBindingSystem.cpp
        AetherEngine/headers/systems/SceneAssetBindingSystem.hpp
)

target_include_directories(AetherEngine
        PUBLIC AetherEngine/headers
        PRIVATE AetherEngine/src
)

target_compile_definitions(AetherEngine PRIVATE ${SMB_PHYSICS_BACKEND_DEFINITION})

if (MSVC)
    target_compile_options(AetherEngine PRIVATE "/Zc:__cplusplus" "/Zc:preprocessor")
endif()

include(CTest)
if (BUILD_TESTING)
    add_executable(aether_security_tests
            tests/SecurityRegressionTests.cpp
    )
    target_link_libraries(aether_security_tests PRIVATE AetherEngine)
    target_compile_definitions(aether_security_tests PRIVATE
            AETHER_TEST_ASSETS_ROOT="${CMAKE_SOURCE_DIR}/assets"
    )
    add_test(NAME security_regressions COMMAND aether_security_tests)

    add_executable(aether_physics_tests
            tests/PhysicsBackendTests.cpp
    )
    target_include_directories(aether_physics_tests PRIVATE AetherEngine/src)
    target_link_libraries(aether_physics_tests PRIVATE AetherEngine)
    add_test(NAME physics_backend_regressions COMMAND aether_physics_tests)

    add_executable(aether_asset_registry_tests
            tests/AssetRegistryTests.cpp
    )
    target_link_libraries(aether_asset_registry_tests PRIVATE AetherEngine)
    target_compile_definitions(aether_asset_registry_tests PRIVATE
            AETHER_TEST_ASSETS_ROOT="${CMAKE_SOURCE_DIR}/assets"
    )
    add_test(NAME asset_registry_regressions COMMAND aether_asset_registry_tests)
endif()

add_executable(smb
        smb/src/core/Smb.cpp
        smb/headers/core/Smb.hpp
        smb/src/main.cpp
)

add_executable(AetherEditor
        AetherEditor/src/main.cpp
        AetherEditor/src/core/AetherEditor.cpp
        AetherEditor/headers/core/AetherEditor.hpp
        AetherEditor/src/ui/EditorGui.cpp
        AetherEditor/headers/ui/EditorGui.hpp
        AetherEditor/src/ui/ImGuiBgfxRenderer.cpp
        AetherEditor/headers/ui/ImGuiBgfxRenderer.hpp
        AetherEditor/headers/core/ETypes.hpp
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
        AetherEditor/headers
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

target_link_libraries(AetherEngine PUBLIC
        SDL3::SDL3
        EnTT::EnTT
        bgfx::bgfx
        glm::glm
        nlohmann_json::nlohmann_json
)

target_link_libraries(AetherEngine PRIVATE
        ${SMB_PHYSICS_BACKEND_TARGET}
)

target_link_libraries(smb PRIVATE
        AetherEngine
)

target_link_libraries(AetherEditor PRIVATE
        AetherEngine
        imgui::imgui
)

if (MSVC)
    set(hardened_targets AetherEngine smb AetherEditor)
    set(hardened_executables smb AetherEditor)
    if (TARGET aether_security_tests)
        list(APPEND hardened_targets aether_security_tests)
        list(APPEND hardened_executables aether_security_tests)
    endif()
    if (TARGET aether_physics_tests)
        list(APPEND hardened_targets aether_physics_tests)
        list(APPEND hardened_executables aether_physics_tests)
    endif()
    if (TARGET aether_asset_registry_tests)
        list(APPEND hardened_targets aether_asset_registry_tests)
        list(APPEND hardened_executables aether_asset_registry_tests)
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
