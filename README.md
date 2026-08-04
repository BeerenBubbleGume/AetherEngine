# smb

`smb` is primarily a C++23 game engine playground. The main artifact is the
reusable `engine` static library; the `smb` executable is a small sample
application used to exercise and validate engine systems.

The engine currently provides SDL3 window/event handling, bgfx rendering,
resource loading, a lightweight EnTT scene wrapper, math helpers, camera
components, mesh/program resources, and action-based input.

## Engine Scope

The repository is organized around building up a compact engine layer:

- Platform integration through SDL3 windows and events.
- Rendering through bgfx with mesh, shader program, and transform submission.
- Scene management through a thin EnTT-backed entity/component wrapper.
- Resource management for meshes and shader programs.
- Math types for vectors, quaternions, transforms, and matrices.
- Action-based input mapping on top of SDL scancodes.

The sample game code exists to keep those systems integrated in a real runtime
loop instead of leaving them as isolated library code.

## Project Layout

- `engine/` - reusable engine headers and implementation; this is the core of
  the repository.
- `smb/` - sample application/game layer using the engine.
- `assets/` - meshes, shader sources, and precompiled shader binaries.
- `third_party/bin/` - checked-in bgfx command-line tools for shader/geometry/texture
  processing.
- `vcpkg.json` - dependency manifest.

## Dependencies

The project uses CMake and vcpkg manifest mode. Current direct dependencies are:

- SDL3
- EnTT
- bgfx
- glm
- Jolt Physics or NVIDIA PhysX
- shaderc

Third-party license notices are collected in [LICENSES.md](LICENSES.md).

## Build

Requirements:

- CMake 3.25 or newer
- A C++23 compiler
- vcpkg

Configure and build with the vcpkg toolchain:

```powershell
cmake -S . -B cmake-build-debug -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build cmake-build-debug --config Debug
```

The physics backend can be selected at configure time:

```powershell
cmake -S . -B cmake-build-debug -DSMB_PHYSICS_BACKEND=Jolt -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
```

Valid values are `Jolt` and `PhysX`. Jolt is the default on macOS, where the
vcpkg PhysX port is not supported; PhysX remains the default elsewhere.

With Visual Studio on Windows, an explicit generator is also fine:

```powershell
cmake -S . -B cmake-build-debug-visual-studio -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build cmake-build-debug-visual-studio --config Debug
```

The build creates an `assets` junction/symlink next to the `smb` executable, so
runtime asset paths resolve from the binary directory.

## Run

Run the sample `smb` executable from the selected CMake build directory. For a
Visual Studio Debug build on Windows:

```powershell
.\cmake-build-debug-visual-studio\Debug\smb.exe
```

## Controls

- `W`, `A`, `S`, `D` - move the controlled mesh.
- Arrow keys - rotate the controlled mesh.
- `R` - reset the controlled mesh transform.
- `Esc` - quit.

## Assets And Shaders

The engine sample currently loads `assets/meshes/bin/bunny.bin` and the
precompiled `basic` shader binaries from `assets/shaders/bin/<platform>/`.
Shader sources live in `assets/shaders/`; bgfx tool binaries are available under
`third_party/bin/` for regenerating shader and geometry assets when needed.

## License

This repository does not currently declare a project-level license. Third-party
dependency notices are listed in [LICENSES.md](LICENSES.md).
