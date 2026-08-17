# AetherEngine / SMB

This repository is a C++23 game-engine playground built around the reusable
`AetherEngine` static library. It also contains two executable front ends:

- `smb` — a small sample game used to exercise the runtime systems.
- `AetherEditor` — an ImGui-based editor shell with an independent scene view.

The project is currently focused on establishing clean engine boundaries,
platform-aware asset loading, hardened native-resource handling, and a compact
ECS/physics/rendering loop that can grow without coupling scene data to a
specific machine or graphics backend.

## Current Features

- SDL3 window management, events, and action-based input.
- bgfx rendering with cameras, render targets, meshes, textures, materials, and
  shader programs.
- EnTT-backed scenes and entities with versioned JSON serialization.
- Platform-aware virtual asset system with stable IDs, typed references and
  handles, dependency loading, caching, unload/reload, and generation checks.
- Runtime loaders for meshes, textures, shaders, and materials.
- Selectable NVIDIA PhysX and Jolt Physics backends.
- Fixed-step physics with bounded frame catch-up.
- ImGui editor integration and a separate editor camera.
- Bounded and validated scene, mesh, texture, and shader inputs.
- Hardened filesystem access for runtime assets and scenes.

## Architecture

The main runtime asset flow is:

```text
Scene AssetId
    -> AssetManager
    -> AssetRegistry
    -> PlatformProfile / variant selection
    -> IAssetLoader
    -> RuntimeAsset
    -> internal GPU resource pool
```

Important ownership boundaries:

- Scenes store stable asset IDs and component-specific metadata. They do not
  store platform-specific filesystem paths or GPU handles.
- `AssetRegistry` is part of the asset subsystem. It maps IDs and aliases to
  dependency metadata and cooked variants for a requested platform profile.
- `AssetManager` owns runtime asset state and exposes typed `AssetRef<T>` and
  `AssetHandle<T>` values to engine systems.
- `RuntimeAsset` is the virtual extension boundary. Runtime objects release
  their low-level GPU resources when their owning entry is unloaded.
- `ResourceManager` is an internal, reference-counted GPU resource pool. It is
  intentionally not exposed through `EngineContext` as a second public asset
  API.
- Materials are assets of their own and load shader/texture dependencies
  through the same manager.

The default scene uses schema version 2 and contains only asset IDs. Physical
paths are isolated in `assets/registry/runtime-assets.json`; material dependency
metadata lives in `assets/materials/`.

## Project Layout

- `AetherEngine/` — reusable engine headers and implementation.
- `AetherEditor/` — editor application and ImGui/bgfx integration.
- `smb/` — sample application/game layer.
- `assets/registry/` — runtime asset manifest and platform variants.
- `assets/materials/` — material assets and their dependency metadata.
- `assets/scenes/` — versioned scene JSON files.
- `assets/meshes/`, `assets/textures/`, `assets/shaders/` — source and cooked
  runtime content.
- `tests/` — security, physics-backend, and asset-registry regression tests.
- `third_party/` — checked-in third-party artifacts; the build does not
  automatically execute the bundled unsigned content tools.
- `vcpkg.json` — dependency manifest.

## Dependencies

The project uses CMake and vcpkg manifest mode. Direct dependencies are:

- SDL3
- EnTT
- bgfx
- glm
- nlohmann/json
- Dear ImGui with docking and SDL3 bindings
- Jolt Physics
- NVIDIA PhysX on non-macOS platforms

Third-party license notices are collected in [LICENSES.md](LICENSES.md).

## Build

Requirements:

- CMake 3.25 or newer
- A C++23 compiler
- vcpkg

Configure and build using the vcpkg toolchain:

```powershell
cmake -S . -B cmake-build-debug `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build cmake-build-debug
```

For Visual Studio 2022 on Windows:

```powershell
cmake -S . -B cmake-build-debug-visual-studio `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build cmake-build-debug-visual-studio --config Debug
```

The physics backend is selected during configuration:

```powershell
cmake -S . -B cmake-build-jolt `
  -DSMB_PHYSICS_BACKEND=Jolt `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
```

Valid values are `PhysX` and `Jolt`. PhysX is the default except on macOS,
where Jolt is used because the vcpkg PhysX port is unavailable.

Both executable targets receive a copied `assets` directory next to the built
binary. Runtime content is deliberately copied instead of being exposed through
a junction or symlink.

## Run

For a Visual Studio Debug build on Windows:

```powershell
.\cmake-build-debug-visual-studio\Debug\smb.exe
.\cmake-build-debug-visual-studio\Debug\AetherEditor.exe
```

Sample controls:

- `W`, `A`, `S`, `D` — move the controlled entity.
- Arrow keys — rotate the controlled entity.
- Right mouse button — rotate using mouse movement.
- Mouse wheel — change the active camera field of view.
- `R` — reset the controlled transform.
- `Esc` — quit.

## Tests

Build with `BUILD_TESTING=ON` (the CTest default) and run:

```powershell
ctest --test-dir cmake-build-debug-visual-studio -C Debug --output-on-failure
```

Current suites cover:

- secure path and native asset parsing regressions;
- the selected physics backend contract;
- asset manifest parsing and platform-variant selection;
- typed runtime handle caching, reference counting, unload, and reload.

## Assets and Platform Support

The engine detects the active OS, CPU architecture, and bgfx renderer and then
selects the best compatible variant from the runtime manifest. Portable assets,
such as the current mesh and KTX texture, use wildcard platform profiles.

The repository currently ships trusted sample shader variants for:

- Windows x64 with Direct3D 11.
- macOS ARM64 with Metal.

The platform model also recognizes Direct3D 12, Vulkan, OpenGL, and OpenGL ES,
but the sample manifest does not yet provide cooked basic-shader variants for
those combinations. Loading fails with an asset error when no compatible
variant exists. Arbitrary compiled shaders from disk are intentionally not
passed to bgfx; the current basic programs use validated, embedded shader
payloads.

Scene schema version 1 is not migrated automatically. Existing scenes must be
converted to schema version 2 and replace physical mesh/material paths with
stable asset IDs.

## Security Notes

Runtime file access is bounded and anchored to trusted asset/scene roots.
Traversal, unsafe rooted paths, symlinks/reparse points, hard-link aliases, and
unsupported native containers are rejected according to the active platform.
The Windows build also enables available compiler/linker hardening and restricts
DLL search paths.

The prior detailed review is recorded in
[SECURITY_AUDIT.md](SECURITY_AUDIT.md). It is a dated audit report, not a formal
proof or a replacement for fuzzing and cross-platform verification. Its open
host ACL finding must be handled outside the repository.

## Near-Term Roadmap

1. Add an importer/cooker pipeline that generates asset IDs, dependency data,
   content hashes, and runtime manifests.
2. Produce trusted D3D12, Vulkan, OpenGL, and OpenGL ES shader variants.
3. Add asynchronous loading, job scheduling, and streaming budgets.
4. Build an editor asset browser with import, reimport, and hot-reload actions.
5. Expand materials beyond the current single-albedo render path.
6. Add schema migration, fuzzing corpora, sanitizers, and CI coverage for all
   supported platform/backend combinations.

## License

This repository does not currently declare a project-level license. Third-party
dependency notices are listed in [LICENSES.md](LICENSES.md).
