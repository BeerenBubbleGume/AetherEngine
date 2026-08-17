//
// Created by drhaz on 21.06.2026.
//

#include "resources/ResourceManager.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

#include "security/PathSecurity.hpp"
#include "security/AssetValidation.hpp"
#include "resources/EmbeddedShaders.hpp"

namespace {
    constexpr std::uint64_t MiB = 1024ULL * 1024ULL;
    constexpr std::uint64_t MaxTextureBytes = 64ULL * MiB;
    constexpr std::uint64_t MaxMeshBytes = 128ULL * MiB;
    constexpr std::uint64_t MaxTotalSourceBytes = 256ULL * MiB;
    constexpr std::size_t MaxMeshes = 48;
    constexpr std::size_t MaxLoadedMeshGroups = 48;
    constexpr std::size_t MaxPrograms = 16;
    constexpr std::size_t MaxTextures = 2048;
    constexpr std::size_t MaxUniqueResourceRequests = 4096;
    constexpr std::size_t MaxRejectionLogs = 32;
    constexpr std::size_t MaxResourcePathLength = 4096;

    constexpr std::array<std::uint8_t, 12> Ktx1Identifier{
        0xab, 0x4b, 0x54, 0x58, 0x20, 0x31, 0x31, 0xbb, 0x0d, 0x0a, 0x1a, 0x0a
    };
    constexpr std::uint32_t KtxLittleEndian = 0x04030201;
    constexpr std::uint32_t MaxTextureDimension = 16384;
    constexpr std::uint32_t MaxTextureMipLevels = 16;
    constexpr std::uint32_t KtxRgba8 = 0x8058;
    constexpr std::uint32_t KtxSrgb8Alpha8 = 0x8c43;
    constexpr std::uint32_t KtxRgba = 0x1908;

    auto fitsBudget(std::uint64_t current, std::uint64_t addition) -> bool {
        return current <= MaxTotalSourceBytes && addition <= MaxTotalSourceBytes - current;
    }

    auto readU32(std::span<const std::uint8_t> bytes, std::size_t offset) -> std::uint32_t {
        std::uint32_t value = 0;
        std::memcpy(&value, bytes.data() + offset, sizeof(value));
        return value;
    }

    auto isKnownShaderAttribute(std::uint16_t id) -> bool {
        switch (id) {
            case 0x0001:
            case 0x0002:
            case 0x0003:
            case 0x0004:
            case 0x0005:
            case 0x0006:
            case 0x000e:
            case 0x000f:
            case 0x0010:
            case 0x0011:
            case 0x0012:
            case 0x0013:
            case 0x0014:
            case 0x0015:
            case 0x0016:
            case 0x0017:
            case 0x0018:
            case 0x0019:
                return true;
            default:
                return false;
        }
    }

    auto expectedPredefinedUniformType(std::string_view name) -> std::optional<std::uint8_t> {
        constexpr std::array MatrixUniforms{
            std::string_view{"u_viewProj"},
            std::string_view{"u_model"}
        };
        if (std::ranges::find(MatrixUniforms, name) != MatrixUniforms.end()) {
            return static_cast<std::uint8_t>(bgfx::UniformType::Mat4);
        }
        return std::nullopt;
    }

    auto hasValidShaderContainer(
        const std::vector<uint8_t>& bytes,
        char expectedStage,
        bgfx::RendererType::Enum renderer
    ) -> bool {
        constexpr std::size_t MaxShaderUniforms = 8;
        constexpr std::uint8_t UniformFragmentBit = 0x10;
        constexpr std::uint8_t UniformSamplerBit = 0x20;
        constexpr std::uint8_t UniformReadOnlyBit = 0x40;
        constexpr std::uint8_t UniformCompareBit = 0x80;
        constexpr std::uint8_t UniformFlagMask = 0xf0;
        constexpr std::uint32_t MaxConstantBufferBytes = 65520;
        constexpr std::uint16_t MaxTextureSamplers = 16;
        struct UniformRange {
            std::uint32_t begin{0};
            std::uint32_t end{0};
        };

        if (bytes.size() < 18 ||
            (expectedStage != 'V' && expectedStage != 'F') ||
            bytes[0] != static_cast<std::uint8_t>(expectedStage) ||
            bytes[1] != 'S' ||
            bytes[2] != 'H') {
            return false;
        }
        const auto version = bytes[3];
        if (version != 11) {
            return false;
        }

        std::size_t offset = 4;
        const auto canRead = [&](std::size_t count) {
            return offset <= bytes.size() && count <= bytes.size() - offset;
        };
        const auto read8 = [&]() -> std::optional<std::uint8_t> {
            if (!canRead(1)) {
                return std::nullopt;
            }
            return bytes[offset++];
        };
        const auto read16 = [&]() -> std::optional<std::uint16_t> {
            if (!canRead(2)) {
                return std::nullopt;
            }
            const auto value = static_cast<std::uint16_t>(bytes[offset]) |
                static_cast<std::uint16_t>(bytes[offset + 1] << 8);
            offset += 2;
            return value;
        };
        const auto read32 = [&]() -> std::optional<std::uint32_t> {
            if (!canRead(4)) {
                return std::nullopt;
            }
            const auto value = readU32(bytes, offset);
            offset += 4;
            return value;
        };
        const auto skip = [&](std::size_t count) {
            if (!canRead(count)) {
                return false;
            }
            offset += count;
            return true;
        };

        if (!read32() || (version >= 6 && !read32())) {
            return false;
        }
        const auto uniformCount = read16();
        if (!uniformCount || *uniformCount > MaxShaderUniforms) {
            return false;
        }
        std::unordered_set<std::string> uniformNames;
        std::array<UniformRange, MaxShaderUniforms> uniformRanges{};
        std::size_t uniformRangeCount = 0;
        for (std::size_t index = 0; index < *uniformCount; ++index) {
            const auto nameSize = read8();
            if (!nameSize || *nameSize == 0 || *nameSize > 64 || !canRead(*nameSize)) {
                return false;
            }
            std::string name{
                reinterpret_cast<const char*>(bytes.data() + offset),
                static_cast<std::size_t>(*nameSize)
            };
            const auto validFirst = [](const unsigned char character) {
                return character == '_' ||
                    (character >= 'A' && character <= 'Z') ||
                    (character >= 'a' && character <= 'z');
            };
            const auto validRest = [&](const unsigned char character) {
                return validFirst(character) || (character >= '0' && character <= '9');
            };
            if (!validFirst(static_cast<unsigned char>(name.front())) ||
                !std::ranges::all_of(name.substr(1), validRest) ||
                !uniformNames.insert(name).second) {
                return false;
            }
            offset += *nameSize;
            const auto type = read8();
            const auto count = read8();
            const auto registerIndex = read16();
            const auto registerCount = read16();
            const auto textureInfo = read16();
            const auto textureFormat = read16();
            if (!type || !count || !registerIndex || !registerCount ||
                !textureInfo || !textureFormat || *count > 64) {
                return false;
            }

            const auto baseType = static_cast<std::uint8_t>(*type & ~UniformFlagMask);
            const auto flags = static_cast<std::uint8_t>(*type & UniformFlagMask);
            const auto requiredFragmentFlag = expectedStage == 'F' ? UniformFragmentBit : 0;
            if ((flags & UniformFragmentBit) != requiredFragmentFlag ||
                (flags & UniformReadOnlyBit) != 0) {
                return false;
            }

            const auto elementCount = std::max<std::uint32_t>(*count, 1);
            const bool isSampler = (flags & UniformSamplerBit) != 0;
            if (isSampler) {
                if (baseType != bgfx::UniformType::Sampler ||
                    name != "s_albedo" || expectedStage != 'F' ||
                    (flags & ~(UniformFragmentBit | UniformSamplerBit | UniformCompareBit)) != 0 ||
                    elementCount != 1) {
                    return false;
                }
                const auto textureComponent = static_cast<std::uint8_t>(*textureInfo & 0xff);
                const auto textureDimension = static_cast<std::uint8_t>(*textureInfo >> 8);
                switch (renderer) {
                    case bgfx::RendererType::Direct3D11:
                    case bgfx::RendererType::Direct3D12:
                        if (*registerIndex >= MaxTextureSamplers ||
                            elementCount > MaxTextureSamplers - *registerIndex ||
                            *registerCount != elementCount ||
                            *textureInfo != 0 || *textureFormat != 0) {
                            return false;
                        }
                        break;
                    case bgfx::RendererType::Vulkan:
                        if (*registerIndex < 2 ||
                            *registerIndex >= 2 + MaxTextureSamplers ||
                            elementCount > 2 + MaxTextureSamplers - *registerIndex ||
                            *registerCount != 0 ||
                            textureComponent > 4 ||
                            textureDimension < 1 || textureDimension > 6 ||
                            *textureFormat >= bgfx::TextureFormat::Count) {
                            return false;
                        }
                        break;
                    case bgfx::RendererType::Metal:
                    case bgfx::RendererType::OpenGL:
                    case bgfx::RendererType::OpenGLES:
                        if (*registerIndex >= MaxTextureSamplers ||
                            elementCount > MaxTextureSamplers - *registerIndex ||
                            textureComponent > 4 ||
                            (textureDimension != 0 &&
                                (textureDimension < 1 || textureDimension > 6)) ||
                            *textureFormat >= bgfx::TextureFormat::Count) {
                            return false;
                        }
                        break;
                    default:
                        return false;
                }
                continue;
            }

            if ((flags & (UniformSamplerBit | UniformReadOnlyBit | UniformCompareBit)) != 0 ||
                *textureInfo != 0 || *textureFormat != 0 ||
                (*registerIndex % 16) != 0) {
                return false;
            }

            const auto predefinedType = expectedPredefinedUniformType(name);
            if (predefinedType) {
                if (baseType != *predefinedType || elementCount != 1) {
                    return false;
                }
            } else if (name != "u_baseColor" || expectedStage != 'F' ||
                baseType != bgfx::UniformType::Vec4 || elementCount != 1) {
                return false;
            }

            std::uint32_t registersPerElement = 0;
            switch (baseType) {
                case bgfx::UniformType::Vec4: registersPerElement = 1; break;
                case bgfx::UniformType::Mat3: registersPerElement = 3; break;
                case bgfx::UniformType::Mat4: registersPerElement = 4; break;
                default: return false;
            }
            const auto expectedRegisterCount = registersPerElement * elementCount;
            const auto rangeBegin = static_cast<std::uint32_t>(*registerIndex);
            const auto rangeBytes = expectedRegisterCount * 16;
            if (*registerCount != expectedRegisterCount ||
                rangeBegin > MaxConstantBufferBytes ||
                rangeBytes > MaxConstantBufferBytes - rangeBegin) {
                return false;
            }
            const UniformRange range{rangeBegin, rangeBegin + rangeBytes};
            for (std::size_t previous = 0; previous < uniformRangeCount; ++previous) {
                if (range.begin < uniformRanges[previous].end &&
                    uniformRanges[previous].begin < range.end) {
                    return false;
                }
            }
            uniformRanges[uniformRangeCount++] = range;
        }

        const auto shaderSize = read32();
        if (!shaderSize || *shaderSize == 0 ||
            !canRead(static_cast<std::size_t>(*shaderSize) + 1)) {
            return false;
        }
        const auto shaderCodeOffset = offset;
        if (renderer == bgfx::RendererType::Direct3D11 ||
            renderer == bgfx::RendererType::Direct3D12) {
            if (*shaderSize < 4 ||
                bytes[shaderCodeOffset] != 'D' || bytes[shaderCodeOffset + 1] != 'X' ||
                bytes[shaderCodeOffset + 2] != 'B' || bytes[shaderCodeOffset + 3] != 'C') {
                return false;
            }
        } else if (renderer == bgfx::RendererType::Vulkan) {
            if (*shaderSize < 4 || readU32(bytes, shaderCodeOffset) != 0x07230203) {
                return false;
            }
        } else if (renderer == bgfx::RendererType::Metal ||
            renderer == bgfx::RendererType::OpenGL ||
            renderer == bgfx::RendererType::OpenGLES) {
            if (bytes[shaderCodeOffset] != '#') {
                return false;
            }
            for (std::size_t index = 0; index < *shaderSize; ++index) {
                const auto character = bytes[shaderCodeOffset + index];
                if (character == 0 ||
                    (character < 0x20 && character != '\n' && character != '\r' && character != '\t')) {
                    return false;
                }
            }
        } else {
            return false;
        }
        offset += *shaderSize;
        if (bytes[offset++] != 0) {
            return false;
        }

        const auto attributeCount = read8();
        if (!attributeCount || *attributeCount > bgfx::Attrib::Count ||
            (expectedStage == 'F' && *attributeCount != 0)) {
            return false;
        }
        std::unordered_set<std::uint16_t> attributes;
        for (std::size_t index = 0; index < *attributeCount; ++index) {
            const auto attribute = read16();
            if (!attribute || !isKnownShaderAttribute(*attribute) ||
                !attributes.insert(*attribute).second) {
                return false;
            }
        }
        const auto constantBufferSize = read16();
        if (!constantBufferSize ||
            *constantBufferSize > MaxConstantBufferBytes ||
            (*constantBufferSize % 16) != 0) {
            return false;
        }
        for (std::size_t index = 0; index < uniformRangeCount; ++index) {
            if (uniformRanges[index].end > *constantBufferSize) {
                return false;
            }
        }
        return offset == bytes.size();
    }

    auto multiplyWithin(std::uint64_t left, std::uint64_t right, std::uint64_t maximum)
        -> std::optional<std::uint64_t> {
        if (left != 0 && right > maximum / left) {
            return std::nullopt;
        }
        const auto product = left * right;
        return product <= maximum ? std::optional<std::uint64_t>{product} : std::nullopt;
    }

    struct SafeKtx2D {
        struct MipRange {
            std::size_t offset{0};
            std::size_t size{0};
        };

        std::uint16_t width{0};
        std::uint16_t height{0};
        bool hasMips{false};
        bool srgb{false};
        std::uint32_t decodedBytes{0};
        std::array<MipRange, MaxTextureMipLevels> mipRanges{};
        std::size_t mipRangeCount{0};
    };

    auto inspectSafeKtx2D(std::span<const std::uint8_t> bytes) -> std::optional<SafeKtx2D> {
        constexpr std::size_t KtxHeaderBytes = 64;
        if (bytes.size() < KtxHeaderBytes ||
            !std::equal(Ktx1Identifier.begin(), Ktx1Identifier.end(), bytes.begin())) {
            return std::nullopt;
        }
        if (readU32(bytes, 12) != KtxLittleEndian) {
            return std::nullopt;
        }

        const auto glType = readU32(bytes, 16);
        const auto glTypeSize = readU32(bytes, 20);
        const auto glFormat = readU32(bytes, 24);
        const auto glInternalFormat = readU32(bytes, 28);
        const auto glBaseInternalFormat = readU32(bytes, 32);
        auto width = readU32(bytes, 36);
        auto height = readU32(bytes, 40);
        const auto depth = readU32(bytes, 44);
        const auto arrayElements = readU32(bytes, 48);
        const auto faces = readU32(bytes, 52);
        const auto mipLevels = readU32(bytes, 56);
        const auto keyValueBytes = readU32(bytes, 60);

        // texturec's RGBA8 KTX files use zero GL type/format fields. Accept
        // only that exact, well-understood encoding and upload the decoded
        // pixels with createTexture2D below; the KTX container is never handed
        // to bgfx's generic DDS/KTX/PVR parser.
        if (glType != 0 || glTypeSize != 1 || glFormat != 0 ||
            (glInternalFormat != KtxRgba8 && glInternalFormat != KtxSrgb8Alpha8) ||
            glBaseInternalFormat != KtxRgba) {
            return std::nullopt;
        }
        if (width == 0 || width > MaxTextureDimension ||
            height == 0 || height > MaxTextureDimension ||
            depth != 0 || arrayElements != 0 || faces != 1 ||
            mipLevels == 0 || mipLevels > MaxTextureMipLevels ||
            keyValueBytes != 0) {
            return std::nullopt;
        }

        std::uint32_t maximumMipLevels = 1;
        for (auto largestDimension = std::max(width, height);
             largestDimension > 1;
             largestDimension >>= 1) {
            ++maximumMipLevels;
        }
        if (mipLevels != 1 && mipLevels != maximumMipLevels) {
            return std::nullopt;
        }

        SafeKtx2D result{
            .width = static_cast<std::uint16_t>(width),
            .height = static_cast<std::uint16_t>(height),
            .hasMips = mipLevels > 1,
            .srgb = glInternalFormat == KtxSrgb8Alpha8
        };
        std::uint64_t offset = KtxHeaderBytes;
        std::uint64_t decodedBytes = 0;
        for (std::uint32_t mip = 0; mip < mipLevels; ++mip) {
            if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint32_t)) {
                return std::nullopt;
            }
            const auto imageSize = readU32(bytes, static_cast<std::size_t>(offset));
            offset += sizeof(std::uint32_t);

            auto pixels = multiplyWithin(width, height, MaxTextureBytes);
            if (!pixels) {
                return std::nullopt;
            }
            const auto mipBytes = multiplyWithin(*pixels, 4, MaxTextureBytes);
            if (!mipBytes) {
                return std::nullopt;
            }

            if (imageSize != *mipBytes ||
                decodedBytes > MaxTextureBytes - *mipBytes ||
                offset > bytes.size() ||
                *mipBytes > bytes.size() - offset) {
                return std::nullopt;
            }
            result.mipRanges[result.mipRangeCount++] = {
                .offset = static_cast<std::size_t>(offset),
                .size = static_cast<std::size_t>(*mipBytes)
            };
            offset += *mipBytes;
            decodedBytes += *mipBytes;
            width = std::max<std::uint32_t>(width >> 1, 1);
            height = std::max<std::uint32_t>(height >> 1, 1);
        }
        if (offset != bytes.size() || decodedBytes > std::numeric_limits<std::uint32_t>::max()) {
            return std::nullopt;
        }
        result.decodedBytes = static_cast<std::uint32_t>(decodedBytes);
        return result;
    }

    auto programCacheKey(
        const std::filesystem::path& vertexShader,
        const std::filesystem::path& fragmentShader
    ) -> std::string {
        std::string key{vertexShader.generic_string()};
        key.push_back('\0');
        key.append(fragmentShader.generic_string());
        return key;
    }

    auto programRequestKey(
        std::string_view vertexShader,
        std::string_view fragmentShader
    ) -> std::string {
        std::string key{vertexShader};
        key.push_back('\0');
        key.append(fragmentShader);
        return key;
    }

    struct TrustedShaderProgram {
        std::span<const std::uint8_t> vertex;
        std::span<const std::uint8_t> fragment;
    };

    auto trustedShaderProgram(
        const std::filesystem::path& assetsRoot,
        const std::filesystem::path& vertexPath,
        const std::filesystem::path& fragmentPath,
        bgfx::RendererType::Enum renderer
    ) -> std::optional<TrustedShaderProgram> {
        std::filesystem::path expectedVertex;
        std::filesystem::path expectedFragment;
        TrustedShaderProgram program{};
        switch (renderer) {
            case bgfx::RendererType::Direct3D11:
            case bgfx::RendererType::Direct3D12:
                expectedVertex = assetsRoot / "shaders/bin/win32/basic_vs.bin";
                expectedFragment = assetsRoot / "shaders/bin/win32/basic_fs.bin";
                program.vertex = AetherEngine::resources::embedded::basic_vs_win32;
                program.fragment = AetherEngine::resources::embedded::basic_fs_win32;
                break;
            case bgfx::RendererType::Metal:
                expectedVertex = assetsRoot / "shaders/bin/osx_arm/basic_vs.bin";
                expectedFragment = assetsRoot / "shaders/bin/osx_arm/basic_fs.bin";
                program.vertex = AetherEngine::resources::embedded::basic_vs_osx_arm;
                program.fragment = AetherEngine::resources::embedded::basic_fs_osx_arm;
                break;
            default:
                return std::nullopt;
        }

        expectedVertex = expectedVertex.lexically_normal();
        expectedFragment = expectedFragment.lexically_normal();
        if (vertexPath != expectedVertex || fragmentPath != expectedFragment) {
            return std::nullopt;
        }
        return program;
    }

    auto createTrustedShader(
        std::span<const std::uint8_t> binary,
        char expectedStage,
        bgfx::RendererType::Enum renderer
    ) -> bgfx::ShaderHandle {
        const std::vector<std::uint8_t> validationCopy{binary.begin(), binary.end()};
        if (!AetherEngine::security::validateShaderContainer(validationCopy, expectedStage, renderer)) {
            return BGFX_INVALID_HANDLE;
        }
        return bgfx::createShader(bgfx::copy(binary.data(), static_cast<std::uint32_t>(binary.size())));
    }
}

namespace AetherEngine::security {
    auto validateShaderContainer(
        const std::vector<std::uint8_t>& bytes,
        char expectedStage,
        bgfx::RendererType::Enum renderer
    ) -> bool {
        return hasValidShaderContainer(bytes, expectedStage, renderer);
    }

    auto validateKtxPayload(const std::vector<std::uint8_t>& bytes) -> bool {
        return inspectSafeKtx2D(bytes).has_value();
    }
}

namespace AetherEngine::resources {
    ResourceManager::ResourceManagerPtr ResourceManager::createResourceManager(
        const std::filesystem::path& assetsRoot
    ) {
        auto canonicalRoot = security::canonicalDirectory(assetsRoot);
        if (!canonicalRoot) {
            std::cerr << "Failed to initialize resource root: " << canonicalRoot.error().message << std::endl;
            return {};
        }
        return ResourceManagerPtr(new ResourceManager(std::move(*canonicalRoot)), ResourceManagerDeleter{});
    }

    auto ResourceManager::loadMesh(std::string_view filename) -> MeshHandle {
        if (filename.empty() || filename.size() > MaxResourcePathLength) {
            return {};
        }
        const std::string requestKey{filename};
        if (m_failedMeshRequests.contains(requestKey)) {
            return {};
        }
        if (!m_seenResourceRequests.contains(requestKey)) {
            if (m_seenResourceRequests.size() >= MaxUniqueResourceRequests) {
                return {};
            }
            m_seenResourceRequests.insert(requestKey);
        }
        const auto normalized = security::normalizeRelativePathWithin(m_assetsRoot, filename);
        if (!normalized) {
            m_failedMeshRequests.insert(requestKey);
            if (m_rejectionLogCount++ < MaxRejectionLogs) {
                std::cerr << "Rejected mesh path: " << normalized.error().message << std::endl;
            }
            return {};
        }
        const std::string key = normalized->generic_string();

        if (const auto cached = m_meshCache.find(key); cached != m_meshCache.end()) {
            const auto index = static_cast<std::size_t>(cached->second - 1);
            if (cached->second > 0 && index < m_meshes.size() && m_meshes[index].object) {
                auto& slot = m_meshes[index];
                if (slot.referenceCount == std::numeric_limits<std::uint32_t>::max()) {
                    return {};
                }
                ++slot.referenceCount;
                return {cached->second};
            }
            m_meshCache.erase(cached);
        }
        if (m_failedMeshes.contains(key)) {
            return {};
        }
        const auto fail = [&]() -> MeshHandle {
            m_failedMeshes.insert(key);
            m_failedMeshRequests.insert(requestKey);
            return {};
        };
        if (m_meshCache.size() >= MaxMeshes) {
            std::cerr << "Mesh resource limit reached" << std::endl;
            return fail();
        }
        const auto source = security::readRegularFileWithin(m_assetsRoot, filename, MaxMeshBytes);
        if (!source || !fitsBudget(m_loadedSourceBytes, source->bytes.size())) {
            std::cerr << "Rejected mesh read or file size" << std::endl;
            return fail();
        }

        graphics::Mesh mesh;
        const auto remainingGroups = MaxLoadedMeshGroups - m_loadedMeshGroups;
        auto loadResult = mesh.loadFromBgfxGeometry(source->bytes, remainingGroups);
        if (!loadResult) {
            std::cerr << "Failed to load mesh '" << key << "': " << loadResult.error().message << std::endl;
            return fail();
        }
        if (!mesh.isValid()) {
            return fail();
        }

        const auto groupCount = mesh.m_groups.size();
        auto freeSlot = std::ranges::find_if(m_meshes, [](const MeshSlot& slot) {
            return !slot.object;
        });
        std::size_t index{};
        if (freeSlot == m_meshes.end()) {
            index = m_meshes.size();
            m_meshes.push_back(MeshSlot{});
        } else {
            index = static_cast<std::size_t>(std::distance(m_meshes.begin(), freeSlot));
        }
        auto& slot = m_meshes[index];
        slot.object.emplace(std::move(mesh));
        slot.cacheKey = key;
        slot.sourceBytes = source->bytes.size();
        slot.groupCount = groupCount;
        slot.referenceCount = 1;
        m_loadedMeshGroups += groupCount;
        const auto id = static_cast<uint32_t>(index + 1);
        m_meshCache[key] = id;
        m_loadedSourceBytes += source->bytes.size();
        return {id};
    }

    auto ResourceManager::loadProgram(
        std::string_view name,
        std::string_view vertexShaderFilename,
        std::string_view fragmentShaderFilename
    ) -> ProgramHandle {
        if (name.empty() || name.size() > 128 ||
            std::ranges::any_of(name, [](unsigned char character) {
                return character < 0x20 || character == 0x7f;
            })) {
            std::cerr << "Rejected shader program name" << std::endl;
            return {};
        }
        if (vertexShaderFilename.empty() || fragmentShaderFilename.empty() ||
            vertexShaderFilename.size() > MaxResourcePathLength ||
            fragmentShaderFilename.size() > MaxResourcePathLength) {
            return {};
        }
        const std::string requestKey = programRequestKey(vertexShaderFilename, fragmentShaderFilename);
        if (m_failedProgramRequests.contains(requestKey)) {
            return {};
        }
        if (!m_seenResourceRequests.contains(requestKey)) {
            if (m_seenResourceRequests.size() >= MaxUniqueResourceRequests) {
                return {};
            }
            m_seenResourceRequests.insert(requestKey);
        }
        const auto vertexShaderPath = security::normalizeRelativePathWithin(
            m_assetsRoot,
            vertexShaderFilename
        );
        const auto fragmentShaderPath = security::normalizeRelativePathWithin(
            m_assetsRoot,
            fragmentShaderFilename
        );
        if (!vertexShaderPath || !fragmentShaderPath) {
            m_failedProgramRequests.insert(requestKey);
            if (m_rejectionLogCount++ < MaxRejectionLogs) {
                std::cerr << "Rejected shader path outside the asset root or to a non-regular file" << std::endl;
            }
            return {};
        }
        const std::string key = programCacheKey(*vertexShaderPath, *fragmentShaderPath);

        if (const auto cached = m_programCache.find(key); cached != m_programCache.end()) {
            const auto index = static_cast<std::size_t>(cached->second - 1);
            if (cached->second > 0 && index < m_programs.size() && m_programs[index].object) {
                auto& slot = m_programs[index];
                if (slot.referenceCount == std::numeric_limits<std::uint32_t>::max()) {
                    return {};
                }
                ++slot.referenceCount;
                return {cached->second};
            }
            m_programCache.erase(cached);
        }
        if (m_failedPrograms.contains(key)) {
            return {};
        }
        const auto fail = [&]() -> ProgramHandle {
            m_failedPrograms.insert(key);
            m_failedProgramRequests.insert(requestKey);
            return {};
        };
        if (m_programCache.size() >= MaxPrograms) {
            std::cerr << "Shader program resource limit reached" << std::endl;
            return fail();
        }
        const auto renderer = bgfx::getRendererType();
        const auto trustedProgram = trustedShaderProgram(
            m_assetsRoot,
            *vertexShaderPath,
            *fragmentShaderPath,
            renderer
        );
        if (!trustedProgram) {
            std::cerr << "Rejected shader program outside the trusted embedded manifest" << std::endl;
            return fail();
        }
        const auto sourceBytes = static_cast<std::uint64_t>(trustedProgram->vertex.size()) +
            trustedProgram->fragment.size();
        if (!fitsBudget(m_loadedSourceBytes, sourceBytes)) {
            return fail();
        }

        bgfx::ShaderHandle vertexShader = createTrustedShader(trustedProgram->vertex, 'V', renderer);
        bgfx::ShaderHandle fragmentShader = createTrustedShader(trustedProgram->fragment, 'F', renderer);
        if (!bgfx::isValid(vertexShader) || !bgfx::isValid(fragmentShader)) {
            if (bgfx::isValid(vertexShader)) {
                bgfx::destroy(vertexShader);
            }
            if (bgfx::isValid(fragmentShader)) {
                bgfx::destroy(fragmentShader);
            }
            return fail();
        }

        const bgfx::ProgramHandle programHandle =
            bgfx::createProgram(vertexShader, fragmentShader, false);
        bgfx::destroy(vertexShader);
        bgfx::destroy(fragmentShader);
        if (!bgfx::isValid(programHandle)) {
            return fail();
        }

        graphics::ShaderProgram program(programHandle);
        auto freeSlot = std::ranges::find_if(m_programs, [](const ProgramSlot& slot) {
            return !slot.object;
        });
        std::size_t index{};
        if (freeSlot == m_programs.end()) {
            index = m_programs.size();
            m_programs.push_back(ProgramSlot{});
        } else {
            index = static_cast<std::size_t>(std::distance(m_programs.begin(), freeSlot));
        }
        auto& slot = m_programs[index];
        slot.object.emplace(std::move(program));
        slot.cacheKey = key;
        slot.sourceBytes = sourceBytes;
        slot.referenceCount = 1;
        const auto id = static_cast<uint32_t>(index + 1);
        m_programCache[key] = id;
        m_loadedSourceBytes += sourceBytes;
        return {id};
    }

    auto ResourceManager::loadTexture(std::string_view filename) -> TextureHandle {
        if (filename.empty() || filename.size() > MaxResourcePathLength) {
            return {};
        }
        const std::string requestKey{filename};
        if (m_failedTextureRequests.contains(requestKey)) {
            return {};
        }
        if (!m_seenResourceRequests.contains(requestKey)) {
            if (m_seenResourceRequests.size() >= MaxUniqueResourceRequests) {
                return {};
            }
            m_seenResourceRequests.insert(requestKey);
        }
        const auto normalized = security::normalizeRelativePathWithin(m_assetsRoot, filename);
        if (!normalized) {
            m_failedTextureRequests.insert(requestKey);
            if (m_rejectionLogCount++ < MaxRejectionLogs) {
                std::cerr << "Rejected texture path: " << normalized.error().message << std::endl;
            }
            return {};
        }
        const std::string key = normalized->generic_string();

        if (const auto cached = m_textureCache.find(key); cached != m_textureCache.end()) {
            const auto index = static_cast<std::size_t>(cached->second - 1);
            if (cached->second > 0 && index < m_textures.size() && m_textures[index].object) {
                auto& slot = m_textures[index];
                if (slot.referenceCount == std::numeric_limits<std::uint32_t>::max()) {
                    return {};
                }
                ++slot.referenceCount;
                return {cached->second};
            }
            m_textureCache.erase(cached);
        }
        if (m_failedTextures.contains(key)) {
            return {};
        }
        const auto fail = [&]() -> TextureHandle {
            m_failedTextures.insert(key);
            m_failedTextureRequests.insert(requestKey);
            return {};
        };
        if (m_textureCache.size() >= MaxTextures) {
            std::cerr << "Texture resource limit reached" << std::endl;
            return fail();
        }
        const auto source = security::readRegularFileWithin(m_assetsRoot, filename, MaxTextureBytes);
        if (!source || !fitsBudget(m_loadedSourceBytes, source->bytes.size())) {
            std::cerr << "Rejected texture read or file size" << std::endl;
            return fail();
        }

        bgfx::TextureHandle textureHandle = loadTextureBinary(source->bytes);
        if (!bgfx::isValid(textureHandle)) {
            return fail();
        }

        graphics::Texture texture(textureHandle);
        auto freeSlot = std::ranges::find_if(m_textures, [](const TextureSlot& slot) {
            return !slot.object;
        });
        std::size_t index{};
        if (freeSlot == m_textures.end()) {
            index = m_textures.size();
            m_textures.push_back(TextureSlot{});
        } else {
            index = static_cast<std::size_t>(std::distance(m_textures.begin(), freeSlot));
        }
        auto& slot = m_textures[index];
        slot.object.emplace(std::move(texture));
        slot.cacheKey = key;
        slot.sourceBytes = source->bytes.size();
        slot.referenceCount = 1;
        const auto id = static_cast<uint32_t>(index + 1);
        m_textureCache[key] = id;
        m_loadedSourceBytes += source->bytes.size();
        return {id};
    }

    auto ResourceManager::getTexture(TextureHandle handle) const -> const graphics::Texture* {
        if (handle.id > 0 && handle.id <= m_textures.size()) {
            const auto& object = m_textures[handle.id - 1].object;
            return object ? &*object : nullptr;
        }
        return nullptr;
    }

    auto ResourceManager::getMesh(MeshHandle handle) const -> const graphics::Mesh* {
        if (handle.id > 0 && handle.id <= m_meshes.size()) {
            const auto& object = m_meshes[handle.id - 1].object;
            return object ? &*object : nullptr;
        }
        return nullptr;
    }

    auto ResourceManager::getProgram(ProgramHandle handle) const -> const graphics::ShaderProgram* {
        if (handle.id > 0 && handle.id <= m_programs.size()) {
            const auto& object = m_programs[handle.id - 1].object;
            return object ? &*object : nullptr;
        }
        return nullptr;
    }

    auto ResourceManager::releaseMesh(MeshHandle handle) -> void {
        if (handle.id == 0 || handle.id > m_meshes.size()) {
            return;
        }
        auto& slot = m_meshes[handle.id - 1];
        if (!slot.object || slot.referenceCount == 0) {
            return;
        }
        if (--slot.referenceCount != 0) {
            return;
        }
        m_meshCache.erase(slot.cacheKey);
        m_loadedSourceBytes -= std::min(m_loadedSourceBytes, slot.sourceBytes);
        m_loadedMeshGroups -= std::min(m_loadedMeshGroups, slot.groupCount);
        slot.object.reset();
        slot.cacheKey.clear();
        slot.sourceBytes = 0;
        slot.groupCount = 0;
    }

    auto ResourceManager::releaseProgram(ProgramHandle handle) -> void {
        if (handle.id == 0 || handle.id > m_programs.size()) {
            return;
        }
        auto& slot = m_programs[handle.id - 1];
        if (!slot.object || slot.referenceCount == 0) {
            return;
        }
        if (--slot.referenceCount != 0) {
            return;
        }
        m_programCache.erase(slot.cacheKey);
        m_loadedSourceBytes -= std::min(m_loadedSourceBytes, slot.sourceBytes);
        slot.object.reset();
        slot.cacheKey.clear();
        slot.sourceBytes = 0;
    }

    auto ResourceManager::releaseTexture(TextureHandle handle) -> void {
        if (handle.id == 0 || handle.id > m_textures.size()) {
            return;
        }
        auto& slot = m_textures[handle.id - 1];
        if (!slot.object || slot.referenceCount == 0) {
            return;
        }
        if (--slot.referenceCount != 0) {
            return;
        }
        m_textureCache.erase(slot.cacheKey);
        m_loadedSourceBytes -= std::min(m_loadedSourceBytes, slot.sourceBytes);
        slot.object.reset();
        slot.cacheKey.clear();
        slot.sourceBytes = 0;
    }

    auto ResourceManager::loadTextureBinary(std::span<const std::uint8_t> bytes) -> bgfx::TextureHandle {
        const auto texture = inspectSafeKtx2D(bytes);
        if (!texture) {
            return BGFX_INVALID_HANDLE;
        }

        const std::uint64_t flags = texture->srgb ? BGFX_TEXTURE_SRGB : BGFX_TEXTURE_NONE;
        const auto* caps = bgfx::getCaps();
        if (caps == nullptr ||
            texture->width > caps->limits.maxTextureSize ||
            texture->height > caps->limits.maxTextureSize ||
            !bgfx::isTextureValid(
                1,
                false,
                1,
                bgfx::TextureFormat::RGBA8,
                flags | BGFX_SAMPLER_NONE
            )) {
            return BGFX_INVALID_HANDLE;
        }

        bgfx::TextureInfo info{};
        bgfx::calcTextureSize(
            info,
            texture->width,
            texture->height,
            1,
            false,
            texture->hasMips,
            1,
            bgfx::TextureFormat::RGBA8
        );
        if (info.storageSize != texture->decodedBytes) {
            return BGFX_INVALID_HANDLE;
        }

        std::vector<std::uint8_t> decoded;
        try {
            decoded.resize(texture->decodedBytes);
        } catch (const std::bad_alloc&) {
            return BGFX_INVALID_HANDLE;
        }
        std::size_t destinationOffset = 0;
        for (std::size_t index = 0; index < texture->mipRangeCount; ++index) {
            const auto& range = texture->mipRanges[index];
            std::memcpy(decoded.data() + destinationOffset, bytes.data() + range.offset, range.size);
            destinationOffset += range.size;
        }

        const auto* memory = bgfx::copy(decoded.data(), texture->decodedBytes);

        const auto handle = bgfx::createTexture2D(
            texture->width,
            texture->height,
            texture->hasMips,
            1,
            bgfx::TextureFormat::RGBA8,
            flags | BGFX_SAMPLER_NONE,
            memory
        );
        if (!bgfx::isValid(handle)) {
            return BGFX_INVALID_HANDLE;
        }
        return handle;
    }

    ResourceManager::ResourceManager(std::filesystem::path assetsRoot)
        : m_assetsRoot(std::move(assetsRoot)) {
    }
} // namespace AetherEngine::resources
