cmake_minimum_required(VERSION 3.25)
project(smb)

set(CMAKE_CXX_STANDARD 23)

find_package(SDL3 CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)

add_library(engine STATIC
        engine/src/window/Window.cpp
        engine/src/Engine.cpp
        engine/headers/Engine.hpp
        engine/src/systems/SInputSystem.cpp
        engine/headers/systems/SInputSystem.hpp
        engine/headers/interfaces/IUpdatable.hpp
        engine/src/systems/SRenderSystem.cpp
        engine/headers/systems/SRenderSystem.hpp
)

target_include_directories(engine PUBLIC engine/headers)

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