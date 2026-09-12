cmake_minimum_required(VERSION 3.25)
project(smb)

set(CMAKE_CXX_STANDARD 23)

set(AETHER_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/projects/Sandbox" CACHE PATH
        "Authoring project opened by the editor and packaged for smb"
)
get_filename_component(AETHER_PROJECT_ROOT "${AETHER_PROJECT_ROOT}" ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
if (NOT IS_DIRECTORY "${AETHER_PROJECT_ROOT}/assets/scenes" OR
        NOT IS_DIRECTORY "${AETHER_PROJECT_ROOT}/assets/shaders" OR
        NOT EXISTS "${AETHER_PROJECT_ROOT}/assets/registry/runtime-assets.json")
    message(FATAL_ERROR "AETHER_PROJECT_ROOT must contain assets/scenes, assets/shaders and assets/registry/runtime-assets.json")
endif()
file(REAL_PATH "${AETHER_PROJECT_ROOT}" AETHER_PROJECT_ROOT)
file(REAL_PATH "${CMAKE_BINARY_DIR}" AETHER_BUILD_ROOT)
set(AETHER_PROJECT_ROOT_KEY "${AETHER_PROJECT_ROOT}")
set(AETHER_BUILD_ROOT_KEY "${AETHER_BUILD_ROOT}")
if (WIN32)
    string(TOLOWER "${AETHER_PROJECT_ROOT_KEY}" AETHER_PROJECT_ROOT_KEY)
    string(TOLOWER "${AETHER_BUILD_ROOT_KEY}" AETHER_BUILD_ROOT_KEY)
endif()
cmake_path(IS_PREFIX AETHER_BUILD_ROOT_KEY "${AETHER_PROJECT_ROOT_KEY}" NORMALIZE AETHER_PROJECT_IN_BUILD)
cmake_path(IS_PREFIX AETHER_PROJECT_ROOT_KEY "${AETHER_BUILD_ROOT_KEY}" NORMALIZE AETHER_BUILD_IN_PROJECT)
if (AETHER_PROJECT_IN_BUILD OR AETHER_BUILD_IN_PROJECT)
    message(FATAL_ERROR "The authoring project and build directory must be separate trees")
endif()

find_package(SDL3 CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

if (APPLE)
    set(AETHER_DEFAULT_PHYSICS_BACKEND "Jolt")
else()
    set(AETHER_DEFAULT_PHYSICS_BACKEND "PhysX")
endif()
set(AETHER_PHYSICS_BACKEND "${AETHER_DEFAULT_PHYSICS_BACKEND}" CACHE STRING
        "Physics backend used by the engine (PhysX or Jolt)"
)
set_property(CACHE AETHER_PHYSICS_BACKEND PROPERTY STRINGS PhysX Jolt)
string(TOLOWER "${AETHER_PHYSICS_BACKEND}" AETHER_PHYSICS_BACKEND_NORMALIZED)

if (AETHER_PHYSICS_BACKEND_NORMALIZED STREQUAL "physx")
    find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)
    set(AETHER_PHYSICS_BACKEND_SOURCES
            AetherEngine/src/physics/physx/PhysXBackend.cpp
            AetherEngine/src/physics/physx/PhysXBackend.hpp
    )
    set(AETHER_PHYSICS_BACKEND_TARGET unofficial::omniverse-physx-sdk::sdk)
    set(AETHER_PHYSICS_BACKEND_DEFINITION AETHER_PHYSICS_BACKEND_PHYSX=1)
elseif (AETHER_PHYSICS_BACKEND_NORMALIZED STREQUAL "jolt")
    find_package(Jolt CONFIG REQUIRED)
    set(AETHER_PHYSICS_BACKEND_SOURCES
            AetherEngine/src/physics/jolt/JoltBackend.cpp
            AetherEngine/src/physics/jolt/JoltBackend.hpp
    )
    set(AETHER_PHYSICS_BACKEND_TARGET Jolt::Jolt)
    set(AETHER_PHYSICS_BACKEND_DEFINITION AETHER_PHYSICS_BACKEND_JOLT=1)
else()
    message(FATAL_ERROR
            "Unsupported AETHER_PHYSICS_BACKEND='${AETHER_PHYSICS_BACKEND}'. "
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
        AetherEngine/src/core/EngineConfig.cpp
        AetherEngine/headers/core/EngineConfig.hpp
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
        ${AETHER_PHYSICS_BACKEND_SOURCES}
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

target_compile_definitions(AetherEngine PRIVATE ${AETHER_PHYSICS_BACKEND_DEFINITION})

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
            AETHER_TEST_ASSETS_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/projects/Sandbox/assets"
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
            AETHER_TEST_ASSETS_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/projects/Sandbox/assets"
    )
    add_test(NAME asset_registry_regressions COMMAND aether_asset_registry_tests)

    add_executable(aether_project_paths_tests tests/ProjectPathsTests.cpp)
    target_link_libraries(aether_project_paths_tests PRIVATE AetherEngine)
    add_test(NAME project_paths_regressions COMMAND aether_project_paths_tests)

    add_executable(aether_scene_write_tests
            tests/SceneWriteTests.cpp
            AetherEngine/src/security/PathSecurity.cpp)
    target_include_directories(aether_scene_write_tests PRIVATE AetherEngine/headers)
    add_test(NAME scene_write_regressions COMMAND aether_scene_write_tests)

    if (WIN32)
        # This test compiles PathSecurity.cpp with local WinAPI wrappers, without
        # introducing fault-injection hooks into the engine library.
        add_executable(aether_scene_write_win32_fault_tests tests/PathSecurityWin32FaultTests.cpp)
        target_include_directories(aether_scene_write_win32_fault_tests PRIVATE AetherEngine/headers)
        foreach(scenario short_writes write_failure partial_failure flush_failure
                metadata_failure collision collisions_exhausted)
            add_test(NAME scene_write_win32_${scenario}
                    COMMAND aether_scene_write_win32_fault_tests ${scenario})
            set_tests_properties(scene_write_win32_${scenario} PROPERTIES TIMEOUT 30)
        endforeach()
    endif()
    add_test(NAME runtime_assets_regressions COMMAND ${CMAKE_COMMAND}
            "-DSTAGING_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/cmake/StageRuntimeAssets.cmake"
            "-DTEST_ROOT=${CMAKE_CURRENT_BINARY_DIR}/runtime-assets-test"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/RuntimeAssetsTests.cmake")
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

set(AETHER_RUNTIME_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")
set_target_properties(smb AetherEditor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${AETHER_RUNTIME_OUTPUT_DIR}")
target_compile_definitions(AetherEditor PRIVATE
        AETHER_DEFAULT_PROJECT_ROOT="${AETHER_PROJECT_ROOT}")
add_custom_target(aether_runtime_assets
        COMMAND ${CMAKE_COMMAND}
                "-DSOURCE_ASSETS=${AETHER_PROJECT_ROOT}/assets"
                "-DRUNTIME_ASSETS=${AETHER_RUNTIME_OUTPUT_DIR}/assets"
                "-DBUILD_ROOT=${CMAKE_CURRENT_BINARY_DIR}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/StageRuntimeAssets.cmake"
        COMMENT "Preparing game assets from ${AETHER_PROJECT_ROOT}"
        VERBATIM
)
add_dependencies(smb aether_runtime_assets)

target_link_libraries(AetherEngine PUBLIC
        SDL3::SDL3
        EnTT::EnTT
        bgfx::bgfx
        glm::glm
        nlohmann_json::nlohmann_json
)

target_link_libraries(AetherEngine PRIVATE
        ${AETHER_PHYSICS_BACKEND_TARGET}
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
    if (TARGET aether_project_paths_tests)
        list(APPEND hardened_targets aether_project_paths_tests)
        list(APPEND hardened_executables aether_project_paths_tests)
    endif()
    foreach(scene_test aether_scene_write_tests aether_scene_write_win32_fault_tests)
        if (TARGET ${scene_test})
            list(APPEND hardened_targets ${scene_test})
            list(APPEND hardened_executables ${scene_test})
        endif()
    endforeach()
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
