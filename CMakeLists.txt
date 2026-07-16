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
        engine/src/systems/SYInputSystem.cpp
        engine/headers/systems/SYInputSystem.hpp
        engine/headers/interfaces/IUpdatable.hpp
        engine/src/systems/SYRenderSystem.cpp
        engine/headers/systems/SYRenderSystem.hpp
        engine/src/graphics/GMesh.cpp
        engine/headers/graphics/GMesh.hpp
        engine/src/graphics/GProgram.cpp
        engine/headers/graphics/GProgram.hpp
        engine/src/resources/RResourceManager.cpp
        engine/headers/resources/RResourceManager.hpp
        engine/src/scene/SCScene.cpp
        engine/headers/scene/SCScene.hpp
        engine/src/scene/SCCamera.cpp
        engine/headers/scene/SCCamera.hpp
        engine/headers/core/IApplication.hpp
        engine/src/core/EngineContext.cpp
        engine/headers/core/EngineContext.hpp
        engine/headers/components/ICameraComponent.hpp
        engine/headers/components/IMeshComponent.hpp
        engine/headers/components/ITransformComponent.hpp
        engine/headers/components/IIdentityComponent.hpp
        engine/headers/scene/Entity.hpp
        engine/src/math/UTypes.cpp
        engine/headers/math/UTypes.hpp
        engine/headers/math/Detail.hpp
        engine/src/math/Detail.cpp
        engine/headers/components/IMaterialComponent.hpp
        engine/headers/math/CameraMatricies.hpp
        engine/src/math/CameraMatricies.cpp
        engine/headers/systems/SYTypes.hpp
        engine/headers/resources/RTypes.hpp
        engine/src/graphics/GTexture.cpp
        engine/headers/graphics/GTexture.hpp
        engine/src/graphics/GRenderTarget.cpp
        engine/headers/graphics/GRenderTarget.hpp
        engine/headers/graphics/GSceneView.hpp
        engine/src/systems/SySceneSerializerSystem.cpp
        engine/headers/systems/SySceneSerializerSystem.hpp
        engine/headers/components/IPhysicsComponents.hpp
        engine/src/systems/SyPhysicsSystem.cpp
        engine/headers/systems/SyPhysicsSystem.hpp
)

target_include_directories(engine PUBLIC engine/headers)

if (MSVC)
    target_compile_options(engine PRIVATE "/Zc:__cplusplus" "/Zc:preprocessor")
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

set(AETHER_IMGUI_SHADER_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets/shaders")
set(AETHER_IMGUI_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/aether_imgui_shaders")
set(AETHER_IMGUI_SHADER_HEADERS)

if (WIN32)
    set(BGFX_SHADERC_EXECUTABLE "${CMAKE_SOURCE_DIR}/third_party/bin/windows/shaderc.exe")
elseif (APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    set(BGFX_SHADERC_EXECUTABLE "${CMAKE_SOURCE_DIR}/third_party/bin/osx/arm/shaderc")
else()
    find_program(BGFX_SHADERC_EXECUTABLE NAMES shaderc REQUIRED)
endif()

if (NOT EXISTS "${BGFX_SHADERC_EXECUTABLE}")
    message(FATAL_ERROR "bgfx shaderc was not found at ${BGFX_SHADERC_EXECUTABLE}")
endif()

function(aether_compile_imgui_shader stage short_name suffix platform profile)
    set(shader_source "${AETHER_IMGUI_SHADER_SOURCE_DIR}/imgui_${short_name}.sc")
    set(shader_output "${AETHER_IMGUI_SHADER_OUTPUT_DIR}/imgui_${short_name}_${suffix}.bin.h")

    add_custom_command(
            OUTPUT "${shader_output}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${AETHER_IMGUI_SHADER_OUTPUT_DIR}"
            COMMAND "${BGFX_SHADERC_EXECUTABLE}"
                    -f "${shader_source}"
                    -o "${shader_output}"
                    --type "${stage}"
                    --platform "${platform}"
                    -p "${profile}"
                    --varyingdef "${AETHER_IMGUI_SHADER_SOURCE_DIR}/imgui_varying.def.sc"
                    -i "${AETHER_IMGUI_SHADER_SOURCE_DIR}"
                    --bin2c "imgui_${short_name}_${suffix}"
            DEPENDS
                    "${shader_source}"
                    "${AETHER_IMGUI_SHADER_SOURCE_DIR}/imgui_varying.def.sc"
                    "${AETHER_IMGUI_SHADER_SOURCE_DIR}/bgfx_shader.sh"
                    "${BGFX_SHADERC_EXECUTABLE}"
            COMMENT "Compiling ImGui ${stage} shader (${suffix})"
            VERBATIM
    )

    set(AETHER_IMGUI_SHADER_HEADERS
            ${AETHER_IMGUI_SHADER_HEADERS}
            "${shader_output}"
            PARENT_SCOPE
    )
endfunction()

if (WIN32)
    aether_compile_imgui_shader(vertex vs dx11 windows s_5_0)
    aether_compile_imgui_shader(fragment fs dx11 windows s_5_0)
    aether_compile_imgui_shader(vertex vs glsl linux 120)
    aether_compile_imgui_shader(fragment fs glsl linux 120)
    aether_compile_imgui_shader(vertex vs essl android 300_es)
    aether_compile_imgui_shader(fragment fs essl android 300_es)
    aether_compile_imgui_shader(vertex vs spv linux spirv)
    aether_compile_imgui_shader(fragment fs spv linux spirv)
elseif (APPLE)
    aether_compile_imgui_shader(vertex vs mtl osx metal)
    aether_compile_imgui_shader(fragment fs mtl osx metal)
    aether_compile_imgui_shader(vertex vs glsl osx 120)
    aether_compile_imgui_shader(fragment fs glsl osx 120)
    aether_compile_imgui_shader(vertex vs essl ios 300_es)
    aether_compile_imgui_shader(fragment fs essl ios 300_es)
    aether_compile_imgui_shader(vertex vs spv osx spirv)
    aether_compile_imgui_shader(fragment fs spv osx spirv)
else()
    aether_compile_imgui_shader(vertex vs glsl linux 120)
    aether_compile_imgui_shader(fragment fs glsl linux 120)
    aether_compile_imgui_shader(vertex vs essl android 300_es)
    aether_compile_imgui_shader(fragment fs essl android 300_es)
    aether_compile_imgui_shader(vertex vs spv linux spirv)
    aether_compile_imgui_shader(fragment fs spv linux spirv)
endif()

target_sources(AetherEditor PRIVATE ${AETHER_IMGUI_SHADER_HEADERS})

target_include_directories(smb PRIVATE smb/headers)
target_include_directories(AetherEditor PRIVATE
        editor/headers
        "${AETHER_IMGUI_SHADER_OUTPUT_DIR}"
)

set(ENGINE_ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")

function(link_engine_assets target_name)
    set(runtime_assets_dir "$<TARGET_FILE_DIR:${target_name}>/assets")
    if (WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf
                "${runtime_assets_dir}"
                COMMAND cmd /C mklink /J
                "$<SHELL_PATH:${runtime_assets_dir}>"
                "$<SHELL_PATH:${ENGINE_ASSETS_SOURCE_DIR}>"
                COMMENT "Creating ${target_name} assets junction"
                VERBATIM
        )
    else()
        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E rm -rf
                "${runtime_assets_dir}"
                COMMAND ${CMAKE_COMMAND} -E create_symlink
                "${ENGINE_ASSETS_SOURCE_DIR}"
                "${runtime_assets_dir}"
                COMMENT "Creating ${target_name} assets symlink"
                VERBATIM
        )
    endif()
endfunction()

link_engine_assets(smb)
link_engine_assets(AetherEditor)

target_link_libraries(engine PUBLIC
        SDL3::SDL3
        EnTT::EnTT
        bgfx::bgfx
        glm::glm
        nlohmann_json::nlohmann_json
        unofficial::omniverse-physx-sdk::sdk
)

target_link_libraries(smb PRIVATE
        engine
)

target_link_libraries(AetherEditor PRIVATE
        engine
        imgui::imgui
)
