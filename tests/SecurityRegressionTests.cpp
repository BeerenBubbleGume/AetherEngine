#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <bgfx/bgfx.h>

#include "graphics/Mesh.hpp"
#include "resources/EmbeddedShaders.hpp"
#include "security/AssetValidation.hpp"
#include "security/PathSecurity.hpp"
#include "systems/SceneSerializer.hpp"

namespace {
    auto readBytes(const std::filesystem::path& path) -> std::vector<std::uint8_t> {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            return {};
        }
        const auto size = file.tellg();
        if (size <= 0) {
            return {};
        }
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
        return file ? bytes : std::vector<std::uint8_t>{};
    }

    auto writeText(const std::filesystem::path& path, std::string_view text) -> bool {
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        file.write(text.data(), static_cast<std::streamsize>(text.size()));
        return file.good();
    }

    auto expect(bool condition, std::string_view message, int& failures) -> void {
        if (!condition) {
            std::cerr << "FAIL: " << message << '\n';
            ++failures;
        }
    }

    auto writeU32(std::vector<std::uint8_t>& bytes, std::size_t offset, std::uint32_t value) -> bool {
        if (offset > bytes.size() || sizeof(value) > bytes.size() - offset) {
            return false;
        }
        bytes[offset] = static_cast<std::uint8_t>(value);
        bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8);
        bytes[offset + 2] = static_cast<std::uint8_t>(value >> 16);
        bytes[offset + 3] = static_cast<std::uint8_t>(value >> 24);
        return true;
    }
}

int main() {
    int failures = 0;
    const std::filesystem::path assetsRoot{AETHER_TEST_ASSETS_ROOT};
    const auto unique = std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()
    );
    const auto testRoot = std::filesystem::temp_directory_path() /
        ("aether-security-tests-" + unique);
    const auto sceneRoot = testRoot / "scenes";
    const auto outsideDirectory = testRoot.parent_path() / ("aether-outside-dir-" + unique);
    std::error_code filesystemError;
    std::filesystem::create_directories(sceneRoot, filesystemError);
    if (filesystemError) {
        std::cerr << "Could not create test directory: " << filesystemError.message() << '\n';
        return 1;
    }
    std::filesystem::create_directory(outsideDirectory, filesystemError);
    if (filesystemError) {
        std::cerr << "Could not create outside test directory: " << filesystemError.message() << '\n';
        return 1;
    }

    expect(engine::security::isSafeSceneName("DefaultScene"), "ordinary scene name", failures);
    expect(!engine::security::isSafeSceneName("../escape"), "scene traversal", failures);
    expect(!engine::security::isSafeSceneName("CON"), "Windows device scene name", failures);
    expect(!engine::security::isSafeSceneName("CONOUT$"), "extended Windows device scene name", failures);
    expect(!engine::security::isSafeSceneName("COM\xc2\xb9"), "superscript Windows device scene name", failures);
    expect(!engine::security::isSafeSceneName("LPT\xc2\xb2.txt"), "superscript device name with extension", failures);

    const auto insideFile = testRoot / "inside.bin";
    const auto outsideFile = testRoot.parent_path() / ("aether-outside-" + unique + ".bin");
    expect(writeText(insideFile, "inside"), "write inside fixture", failures);
    expect(writeText(outsideFile, "outside"), "write outside fixture", failures);
    const auto canonicalRoot = engine::security::canonicalDirectory(testRoot);
    expect(canonicalRoot.has_value(), "canonical test root", failures);
    if (canonicalRoot) {
        expect(
            engine::security::resolveRegularFileWithin(*canonicalRoot, "inside.bin").has_value(),
            "file inside asset root",
            failures
        );
        const auto secureInside = engine::security::readRegularFileWithin(
            *canonicalRoot,
            "inside.bin",
            64
        );
        expect(
            secureInside && std::string{
                reinterpret_cast<const char*>(secureInside->bytes.data()),
                secureInside->bytes.size()
            } == "inside",
            "same-handle bounded file read",
            failures
        );
        constexpr std::string_view SecureWriteContents{"secure-write"};
        const auto secureWrite = engine::security::writeRegularFileWithin(
            *canonicalRoot,
            "secure-write.bin",
            std::span<const std::uint8_t>{
                reinterpret_cast<const std::uint8_t*>(SecureWriteContents.data()),
                SecureWriteContents.size()
            },
            64
        );
        if (!secureWrite) {
            std::cerr << "secure write error: " << secureWrite.error().message << '\n';
        }
        expect(secureWrite.has_value(), "same-handle bounded file write", failures);
        const auto secureWrittenFile = engine::security::readRegularFileWithin(
            *canonicalRoot,
            "secure-write.bin",
            64
        );
        expect(
            secureWrittenFile && std::string{
                reinterpret_cast<const char*>(secureWrittenFile->bytes.data()),
                secureWrittenFile->bytes.size()
            } == SecureWriteContents,
            "read securely written file",
            failures
        );
        const auto hardLink = testRoot / "inside-hardlink.bin";
        filesystemError.clear();
        std::filesystem::create_hard_link(insideFile, hardLink, filesystemError);
        if (!filesystemError) {
            expect(
                !engine::security::readRegularFileWithin(
                    *canonicalRoot,
                    "inside-hardlink.bin",
                    64
                ).has_value(),
                "hard-link alias rejection",
                failures
            );
            std::filesystem::remove(hardLink, filesystemError);
        }
        expect(
            engine::security::resolveDirectoryWithin(*canonicalRoot, "scenes").has_value(),
            "directory inside asset root",
            failures
        );
        expect(
            !engine::security::resolveDirectoryWithin(*canonicalRoot, "../").has_value(),
            "directory traversal outside asset root",
            failures
        );
        const auto linkedDirectory = testRoot / "linked-dir";
        filesystemError.clear();
        std::filesystem::create_directory_symlink(outsideDirectory, linkedDirectory, filesystemError);
        if (!filesystemError) {
            expect(
                !engine::security::canonicalDirectory(linkedDirectory).has_value(),
                "symlink or reparse-point asset root rejection",
                failures
            );
            expect(
                !engine::security::resolveDirectoryWithin(*canonicalRoot, "linked-dir").has_value(),
                "directory symlink or reparse-point rejection",
                failures
            );
            expect(
                !engine::security::readRegularFileWithin(
                    *canonicalRoot,
                    "linked-dir/outside.bin",
                    64
                ).has_value(),
                "same-handle read rejects linked directory",
                failures
            );
            std::filesystem::remove(linkedDirectory, filesystemError);
        }
        const auto linkedFile = testRoot / "linked-file.bin";
        filesystemError.clear();
        std::filesystem::create_symlink(outsideFile, linkedFile, filesystemError);
        if (!filesystemError) {
            expect(
                !engine::security::readRegularFileWithin(
                    *canonicalRoot,
                    "linked-file.bin",
                    64
                ).has_value(),
                "same-handle read rejects file symlink",
                failures
            );
            expect(
                !engine::security::writeRegularFileWithin(
                    *canonicalRoot,
                    "linked-file.bin",
                    std::span<const std::uint8_t>{
                        reinterpret_cast<const std::uint8_t*>(SecureWriteContents.data()),
                        SecureWriteContents.size()
                    },
                    64
                ).has_value(),
                "same-handle write rejects file symlink",
                failures
            );
            const auto outsideAfterRejectedWrite = readBytes(outsideFile);
            expect(
                std::string{
                    reinterpret_cast<const char*>(outsideAfterRejectedWrite.data()),
                    outsideAfterRejectedWrite.size()
                } == "outside",
                "rejected symlink write leaves outside target unchanged",
                failures
            );
            std::filesystem::remove(linkedFile, filesystemError);
        }
        expect(
            !engine::security::resolveRegularFileWithin(*canonicalRoot, outsideFile.string()).has_value(),
            "absolute path outside asset root",
            failures
        );
        expect(
            !engine::security::resolveRegularFileWithin(*canonicalRoot, "../" + outsideFile.filename().string()).has_value(),
            "relative traversal outside asset root",
            failures
        );
#if defined(_WIN32)
        expect(
            !engine::security::resolveRegularFileWithin(*canonicalRoot, "CON").has_value(),
            "Windows device resource path rejection",
            failures
        );
        expect(
            !engine::security::resolveRegularFileWithin(*canonicalRoot, "inside.bin.").has_value(),
            "Windows trailing-dot resource path rejection",
            failures
        );
        expect(
            !engine::security::resolveRegularFileWithin(*canonicalRoot, R"(\\server\share\payload.bin)").has_value(),
            "UNC path rejection",
            failures
        );
        expect(
            !engine::security::resolveRegularFileWithin(*canonicalRoot, "inside.bin:stream").has_value(),
            "alternate data stream rejection",
            failures
        );
        expect(
            !engine::security::resolveRegularFileWithin(*canonicalRoot, "COM\xc2\xb9").has_value(),
            "superscript Windows device resource path rejection",
            failures
        );
#endif
    }

    const auto vertexShader = readBytes(assetsRoot / "shaders/bin/win32/basic_vs.bin");
    const auto fragmentShader = readBytes(assetsRoot / "shaders/bin/win32/basic_fs.bin");
    expect(!vertexShader.empty() && !fragmentShader.empty(), "read shader fixtures", failures);
    expect(
        vertexShader.size() == std::size(engine::resources::embedded::basic_vs_win32) &&
        std::equal(
            vertexShader.begin(),
            vertexShader.end(),
            std::begin(engine::resources::embedded::basic_vs_win32)
        ),
        "embedded vertex shader matches trusted source",
        failures
    );
    expect(
        fragmentShader.size() == std::size(engine::resources::embedded::basic_fs_win32) &&
        std::equal(
            fragmentShader.begin(),
            fragmentShader.end(),
            std::begin(engine::resources::embedded::basic_fs_win32)
        ),
        "embedded fragment shader matches trusted source",
        failures
    );
    expect(
        engine::security::validateShaderContainer(
            vertexShader,
            'V',
            bgfx::RendererType::Direct3D11
        ),
        "valid vertex shader",
        failures
    );
    expect(
        engine::security::validateShaderContainer(
            fragmentShader,
            'F',
            bgfx::RendererType::Direct3D11
        ),
        "valid fragment shader",
        failures
    );
    auto maliciousShader = fragmentShader;
    constexpr std::string_view UniformName{"u_baseColor"};
    const auto uniformPosition = std::search(
        maliciousShader.begin(),
        maliciousShader.end(),
        UniformName.begin(),
        UniformName.end()
    );
    if (uniformPosition != maliciousShader.end()) {
        const auto recordOffset = static_cast<std::size_t>(uniformPosition - maliciousShader.begin()) +
            UniformName.size();
        maliciousShader[recordOffset + 2] = 0xff;
        maliciousShader[recordOffset + 3] = 0xff;
        expect(
            !engine::security::validateShaderContainer(
                maliciousShader,
                'F',
                bgfx::RendererType::Direct3D11
            ),
            "out-of-range shader register",
            failures
        );
    } else {
        expect(false, "find shader uniform fixture", failures);
    }
    expect(
        !engine::security::validateShaderContainer(
            fragmentShader,
            'F',
            bgfx::RendererType::Vulkan
        ),
        "backend-mismatched shader payload",
        failures
    );

    auto texture = readBytes(assetsRoot / "textures/bin/brick.ktx");
    expect(!texture.empty(), "read KTX fixture", failures);
    expect(engine::security::validateKtxPayload(texture), "valid KTX texture", failures);
    auto cubeTexture = texture;
    expect(writeU32(cubeTexture, 52, 6), "mutate KTX face count", failures);
    expect(!engine::security::validateKtxPayload(cubeTexture), "cube KTX rejection", failures);
    auto volumeTexture = texture;
    expect(writeU32(volumeTexture, 44, 1), "mutate KTX depth", failures);
    expect(!engine::security::validateKtxPayload(volumeTexture), "3D KTX rejection", failures);
    auto arrayTexture = texture;
    expect(writeU32(arrayTexture, 48, 1), "mutate KTX array count", failures);
    expect(!engine::security::validateKtxPayload(arrayTexture), "array KTX rejection", failures);
    if (texture.size() >= 40) {
        writeU32(texture, 36, 0x7fffffff);
        expect(!engine::security::validateKtxPayload(texture), "oversized KTX dimensions", failures);
    }

    auto serializer = engine::systems::SceneSerializer::createSceneSerializer(sceneRoot);
    std::string deepScene = R"({"version":1,"name":"Deep","entities":)";
    deepScene.append(65, '[');
    deepScene.append(65, ']');
    deepScene.push_back('}');
    expect(writeText(sceneRoot / "Deep.scene.json", deepScene), "write deep scene", failures);
    expect(!serializer->deserializeScene("Deep"), "deep JSON rejection", failures);

    constexpr std::string_view DuplicateScene =
        R"({"version":1,"name":"Duplicate","entities":[{"id":"same"},{"id":"same"}]})";
    expect(writeText(sceneRoot / "Duplicate.scene.json", DuplicateScene), "write duplicate scene", failures);
    expect(!serializer->deserializeScene("Duplicate"), "duplicate entity id rejection", failures);

    bgfx::Init bgfxInit{};
    bgfxInit.type = bgfx::RendererType::Noop;
    bgfxInit.resolution.width = 1;
    bgfxInit.resolution.height = 1;
    if (bgfx::init(bgfxInit)) {
        {
            auto resources = engine::resources::ResourceManager::createResourceManager(assetsRoot);
            expect(resources != nullptr, "create resource manager", failures);
            if (resources) {
                const auto textureHandle = resources->loadTexture(
                    "textures/bin/brick.ktx"
                );
                expect(textureHandle.isValid(), "upload validated 2D KTX without generic parser", failures);
            }

            engine::graphics::Mesh validMesh;
            const auto validMeshBytes = readBytes(assetsRoot / "meshes/bin/bunny.bin");
            const auto result = validMesh.loadFromBgfxGeometry(
                validMeshBytes,
                32
            );
            expect(result.has_value() && validMesh.isValid(), "valid bunny mesh", failures);

            auto malformedMesh = readBytes(assetsRoot / "meshes/bin/bunny.bin");
            constexpr std::size_t FirstLayoutComponentCount = 115;
            if (malformedMesh.size() > FirstLayoutComponentCount) {
                malformedMesh[FirstLayoutComponentCount] = 0;
                engine::graphics::Mesh invalidMesh;
                expect(
                    !invalidMesh.loadFromBgfxGeometry(malformedMesh, 32),
                    "invalid vertex layout rejection",
                    failures
                );
            } else {
                expect(false, "mesh fixture layout offset", failures);
            }
        }
        bgfx::shutdown();
    } else {
        expect(false, "initialize bgfx Noop for mesh tests", failures);
    }

    std::filesystem::remove_all(testRoot, filesystemError);
    std::filesystem::remove(outsideFile, filesystemError);
    std::filesystem::remove_all(outsideDirectory, filesystemError);
    if (failures != 0) {
        std::cerr << failures << " security regression test(s) failed\n";
        return 1;
    }
    std::cout << "All security regression tests passed\n";
    return 0;
}
