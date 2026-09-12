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

The default project scene uses schema version 2 and contains only asset IDs.
Physical paths are isolated in the project's `assets/registry/runtime-assets.json`;
material dependency metadata lives in its `assets/materials/`.

## Project Layout

- `AetherEngine/` — reusable engine headers and implementation.
- `AetherEditor/` — editor application and ImGui/bgfx integration.
- `smb/` — sample application/game layer.
- `assets/templates/DefaultScene.scene.json` — starter scene template.
- `assets/shaders/` — engine/editor shader support and embedded ImGui shaders.
- `projects/Sandbox/` — the included authoring project, stored in version control.
- `projects/Sandbox/assets/scenes/Main.scene.json` — editable project scene.
- `projects/Sandbox/assets/registry/`, `materials/`, `meshes/`, `textures/`,
  `shaders/` — the project's manifest, dependencies, source and cooked content.
- `cmake/` — preparation of the disposable runtime asset copy.
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
  -DAETHER_PHYSICS_BACKEND=Jolt `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
```

Valid values are `PhysX` and `Jolt`. PhysX is the default except on macOS,
where Jolt is used because the vcpkg PhysX port is unavailable.

The default authoring project is `projects/Sandbox`. Select another existing
project at configure time with `-DAETHER_PROJECT_ROOT="D:/projects/MyGame"`.
It must contain `assets/scenes/`, `assets/shaders/` and
`assets/registry/runtime-assets.json`. Keep projects outside the build directory
and put the build directory outside the selected authoring project.

The single `aether_runtime_assets` target copies the selected project's assets
next to `smb`. Building `smb` also prepares assets when only scene/content files
have changed. Stale files are removed only from the checked build destination.
To refresh just the game content, run:

```powershell
cmake --build cmake-build-debug-visual-studio --config Debug --target aether_runtime_assets
```

Building `AetherEditor` alone does not copy assets. The editor opens the authoring
project directly. Content is never linked into the build directory.

## Authoring and Runtime Data

```text
assets/templates/DefaultScene.scene.json       starter template
projects/Sandbox/assets/scenes/Main.scene.json authoring scene
cmake-build-.../Debug/assets/scenes/Main.scene.json game copy
```

The included Sandbox scene was created once from the template. Opening the
editor, configuring CMake and rebuilding never seed or overwrite project files
from templates. Template changes do not propagate into existing projects.
New-project creation UI/commands are not implemented yet; another project can
currently be prepared by copying the complete Sandbox project to a new directory.

`Engine::initEngine(EngineInitConfig)` accepts an explicit `projectRoot` and a
`startupScene` (default `Main`). With a project root, the asset manager and scene
serializer use that project's assets. With an empty project root, as in `smb`,
they use the package beside the executable. `EngineContext::paths` exposes the
resolved roots. Missing/invalid projects and scenes report an error; the engine
does not fall back to another project or replace a broken scene with an empty one.

Authoring changes belong in the project directory and version control. The
runtime copy is disposable and is not a location for authoring or persistent game
saves. Atomic scene writes and an editor document model remain separate work.

## Run

For a Visual Studio Debug build on Windows:

```powershell
.\cmake-build-debug-visual-studio\Debug\smb.exe
.\cmake-build-debug-visual-studio\Debug\AetherEditor.exe
```

The editor defaults to the project selected by CMake. Override it at launch:

```powershell
.\cmake-build-debug-visual-studio\Debug\AetherEditor.exe --project "D:\projects\MyGame" --scene Main
```

`--scene` takes a scene name without `.scene.json`. An explicit relative project
path is resolved against the launch working directory; the configured default
is absolute, so launching from an IDE or another directory works as well.
Use `--help` for usage. A launch override affects only the editor; to package that
project for `smb`, also select it with `AETHER_PROJECT_ROOT` in CMake. The sample
game starts `Main` and expects the sample `player` entity.

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
- typed runtime handle caching, reference counting, unload, and loading again;
- project/runtime path selection and saving author edits without touching templates
  or the game package;
- scene file creation and replacement, byte limits, unsafe paths, preservation of
  original data on failure, and temporary-file cleanup;
- on Windows, file identity after replacement, blocked readers, and injected WinAPI failures for
  writes, flushing and metadata checks, plus short writes and temporary-name collisions;
- repeatable runtime copying, stale-file removal and rejection of unsafe copy
  destinations.

Run just the scene-write tests with:

```powershell
cmake --build cmake-build-debug-visual-studio --config Debug --target aether_scene_write_tests aether_scene_write_win32_fault_tests
ctest --test-dir cmake-build-debug-visual-studio -C Debug -R scene_write --output-on-failure
```

The fault-test target is Windows-only; omit it on other platforms. Both targets
compile the current path-security implementation without linking the renderer or
physics backend. Fault injection is confined to the test executable. Each test
uses an isolated temporary directory; symlink checks report a skip if creation is
unavailable. These tests check replacement behavior and failure handling, not
durability across power loss.

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
