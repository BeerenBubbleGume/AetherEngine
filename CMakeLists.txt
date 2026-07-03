cmake_minimum_required(VERSION 3.25)
project(smb)

set(CMAKE_CXX_STANDARD 23)

find_package(SDL3 CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-omniverse-physx-sdk CONFIG REQUIRED)

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

target_include_directories(smb PRIVATE smb/headers)

set(ENGINE_ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")
set(ENGINE_ASSETS_RUNTIME_DIR "$<TARGET_FILE_DIR:smb>/assets")

if (WIN32)
    add_custom_command(TARGET smb POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf
            "${ENGINE_ASSETS_RUNTIME_DIR}"
            COMMAND cmd /C mklink /J
            "$<SHELL_PATH:${ENGINE_ASSETS_RUNTIME_DIR}>"
            "$<SHELL_PATH:${ENGINE_ASSETS_SOURCE_DIR}>"
            COMMENT "Creating engine assets junction"
            VERBATIM
    )
else()
    add_custom_command(TARGET smb POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -rf
            "${ENGINE_ASSETS_RUNTIME_DIR}"
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            "${ENGINE_ASSETS_SOURCE_DIR}"
            "${ENGINE_ASSETS_RUNTIME_DIR}"
            COMMENT "Creating engine assets symlink"
            VERBATIM
    )
endif()

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
