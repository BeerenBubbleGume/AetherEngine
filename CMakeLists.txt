cmake_minimum_required(VERSION 3.25)
project(smb)

set(CMAKE_CXX_STANDARD 23)

find_package(SDL3 CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)

if (MSVC)
    add_compile_options("/Zc:__cplusplus" "/Zc:preprocessor")
endif()
if (MSVC)
    add_compile_options("/permissive-")
endif()

add_library(engine STATIC
        engine/src/window/Window.cpp
        engine/src/Engine.cpp
        engine/headers/Engine.hpp
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
)

target_include_directories(engine PUBLIC engine/headers)

if (MSVC)
    target_compile_options(engine PRIVATE "/Zc:__cplusplus" "/Zc:preprocessor")
endif()

add_executable(smb
        engine/src/main.cpp
)

target_link_libraries(engine PUBLIC
        SDL3::SDL3
        EnTT::EnTT
        bgfx::bgfx
)

target_link_libraries(smb PRIVATE
        engine
)