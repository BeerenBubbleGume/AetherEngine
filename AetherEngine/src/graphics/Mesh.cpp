//
// Created by drhaz on 20.06.2026.
//

#include "graphics/Mesh.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <new>
#include <optional>

namespace {
    constexpr uint32_t kChunkVertexBuffer = BX_MAKEFOURCC('V', 'B', ' ', 0x1);
    constexpr uint32_t kChunkVertexBufferCompressed = BX_MAKEFOURCC('V', 'B', 'C', 0x0);
    constexpr uint32_t kChunkIndexBuffer = BX_MAKEFOURCC('I', 'B', ' ', 0x0);
    constexpr uint32_t kChunkIndexBufferCompressed = BX_MAKEFOURCC('I', 'B', 'C', 0x1);
    constexpr uint32_t kChunkPrimitive = BX_MAKEFOURCC('P', 'R', 'I', 0x0);
    constexpr std::uint64_t MiB = 1024ULL * 1024ULL;
    constexpr std::uint64_t MaxMeshFileBytes = 256ULL * MiB;
    constexpr std::uint64_t MaxMeshBufferBytes = 64ULL * MiB;
    constexpr std::size_t MaxMeshGroups = 32;
    constexpr std::uint16_t MaxVertexStride = 2048;

    auto attributeFromId(std::uint16_t id) -> std::optional<bgfx::Attrib::Enum> {
        switch (id) {
            case 0x0001: return bgfx::Attrib::Position;
            case 0x0002: return bgfx::Attrib::Normal;
            case 0x0003: return bgfx::Attrib::Tangent;
            case 0x0004: return bgfx::Attrib::Bitangent;
            case 0x0005: return bgfx::Attrib::Color0;
            case 0x0006: return bgfx::Attrib::Color1;
            case 0x0018: return bgfx::Attrib::Color2;
            case 0x0019: return bgfx::Attrib::Color3;
            case 0x000e: return bgfx::Attrib::Indices;
            case 0x000f: return bgfx::Attrib::Weight;
            case 0x0010: return bgfx::Attrib::TexCoord0;
            case 0x0011: return bgfx::Attrib::TexCoord1;
            case 0x0012: return bgfx::Attrib::TexCoord2;
            case 0x0013: return bgfx::Attrib::TexCoord3;
            case 0x0014: return bgfx::Attrib::TexCoord4;
            case 0x0015: return bgfx::Attrib::TexCoord5;
            case 0x0016: return bgfx::Attrib::TexCoord6;
            case 0x0017: return bgfx::Attrib::TexCoord7;
            default: return std::nullopt;
        }
    }

    auto attributeTypeFromId(std::uint16_t id) -> std::optional<bgfx::AttribType::Enum> {
        switch (id) {
            case 0x0001: return bgfx::AttribType::Uint8;
            case 0x0005: return bgfx::AttribType::Uint10;
            case 0x0002: return bgfx::AttribType::Int16;
            case 0x0003: return bgfx::AttribType::Half;
            case 0x0004: return bgfx::AttribType::Float;
            default: return std::nullopt;
        }
    }

    auto readSafeVertexLayout(
        bx::MemoryReader& reader,
        bgfx::VertexLayout& layout,
        bx::Error& error
    ) -> bool {
        struct AttributeDescriptor {
            bgfx::Attrib::Enum attribute{};
            bgfx::AttribType::Enum type{};
            std::uint16_t offset{0};
            std::uint16_t size{0};
            std::uint8_t componentCount{0};
            bool normalized{false};
            bool asInteger{false};
        };
        constexpr std::array<std::array<std::uint8_t, 4>, bgfx::AttribType::Count> AttributeSizes{{
            {{1, 2, 4, 4}},
            {{4, 4, 4, 4}},
            {{2, 4, 8, 8}},
            {{2, 4, 8, 8}},
            {{4, 8, 12, 16}}
        }};

        uint8_t attributeCount = 0;
        uint16_t declaredStride = 0;
        if (reader.remaining() < 3 ||
            sizeof(attributeCount) != bx::read(&reader, attributeCount, &error) ||
            sizeof(declaredStride) != bx::read(&reader, declaredStride, &error) ||
            !error.isOk() ||
            attributeCount == 0 ||
            attributeCount > bgfx::Attrib::Count ||
            declaredStride == 0 ||
            declaredStride > MaxVertexStride) {
            return false;
        }

        constexpr std::size_t SerializedAttributeBytes =
            sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t) +
            sizeof(uint16_t) + sizeof(uint8_t) + sizeof(uint8_t);
        if (static_cast<std::uint64_t>(attributeCount) * SerializedAttributeBytes >
            static_cast<std::uint64_t>(reader.remaining())) {
            return false;
        }

        std::array<bool, bgfx::Attrib::Count> seen{};
        std::array<AttributeDescriptor, bgfx::Attrib::Count> descriptors{};
        for (uint32_t index = 0; index < attributeCount; ++index) {
            uint16_t declaredOffset = 0;
            uint16_t attributeId = 0;
            uint8_t componentCount = 0;
            uint16_t typeId = 0;
            uint8_t normalized = 0;
            uint8_t asInteger = 0;
            bx::read(&reader, declaredOffset, &error);
            bx::read(&reader, attributeId, &error);
            bx::read(&reader, componentCount, &error);
            bx::read(&reader, typeId, &error);
            bx::read(&reader, normalized, &error);
            bx::read(&reader, asInteger, &error);

            const auto attribute = attributeFromId(attributeId);
            const auto type = attributeTypeFromId(typeId);
            if (!error.isOk() || !attribute || !type ||
                componentCount == 0 || componentCount > 4 ||
                normalized > 1 || asInteger > 1 ||
                seen[static_cast<std::size_t>(*attribute)]) {
                return false;
            }

            const auto attributeSize = AttributeSizes[static_cast<std::size_t>(*type)][componentCount - 1];
            if (declaredOffset > declaredStride || attributeSize > declaredStride - declaredOffset) {
                return false;
            }
            seen[static_cast<std::size_t>(*attribute)] = true;
            descriptors[index] = {
                .attribute = *attribute,
                .type = *type,
                .offset = declaredOffset,
                .size = attributeSize,
                .componentCount = componentCount,
                .normalized = normalized != 0,
                .asInteger = asInteger != 0
            };
        }

        std::ranges::sort(
            descriptors.begin(),
            descriptors.begin() + attributeCount,
            {},
            &AttributeDescriptor::offset
        );
        for (std::size_t index = 1; index < attributeCount; ++index) {
            const auto previousEnd = static_cast<std::uint32_t>(descriptors[index - 1].offset) +
                descriptors[index - 1].size;
            if (descriptors[index].offset < previousEnd) {
                return false;
            }
        }

        layout.begin();
        std::uint16_t cursor = 0;
        for (std::size_t index = 0; index < attributeCount; ++index) {
            const auto& descriptor = descriptors[index];
            auto gap = static_cast<std::uint16_t>(descriptor.offset - cursor);
            while (gap > 0) {
                const auto chunk = static_cast<std::uint8_t>(std::min<std::uint16_t>(gap, 255));
                layout.skip(chunk);
                gap = static_cast<std::uint16_t>(gap - chunk);
            }
            layout.add(
                descriptor.attribute,
                descriptor.componentCount,
                descriptor.type,
                descriptor.normalized,
                descriptor.asInteger
            );
            cursor = static_cast<std::uint16_t>(descriptor.offset + descriptor.size);
        }
        auto trailing = static_cast<std::uint16_t>(declaredStride - cursor);
        while (trailing > 0) {
            const auto chunk = static_cast<std::uint8_t>(std::min<std::uint16_t>(trailing, 255));
            layout.skip(chunk);
            trailing = static_cast<std::uint16_t>(trailing - chunk);
        }
        layout.end();

        return layout.has(bgfx::Attrib::Position) && layout.getStride() == declaredStride;
    }

    auto isValidGroup(const AetherEngine::graphics::MeshGroup& group) -> bool {
        return bgfx::isValid(group.vbh) && bgfx::isValid(group.ibh);
    }
}


namespace AetherEngine::graphics {

    Mesh::Mesh(Mesh &&other) noexcept {
        this->m_groups = std::move(other.m_groups);
        this->m_layout = std::move(other.m_layout);

        other.m_groups.clear();
        other.m_layout = {};
    }

    Mesh & Mesh::operator=(Mesh &&other) noexcept {
        if (this == &other) {
            return *this;
        }

        destroyHandles();

        this->m_groups = std::move(other.m_groups);
        this->m_layout = std::move(other.m_layout);

        other.m_groups.clear();
        other.m_layout = {};
        return *this;
    }

    void Mesh::createFromVertices(std::span<const AetherEngine::math::Vec3> vertices, std::span<const uint16_t> indices) {
        destroyHandles();
        if (vertices.empty() || indices.empty() ||
            vertices.size_bytes() > MaxMeshBufferBytes ||
            indices.size_bytes() > MaxMeshBufferBytes ||
            vertices.size_bytes() > std::numeric_limits<uint32_t>::max() ||
            indices.size_bytes() > std::numeric_limits<uint32_t>::max()) {
            return;
        }

        m_layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .end();

        MeshGroup group;
        group.vbh = bgfx::createVertexBuffer(
            bgfx::copy(vertices.data(), static_cast<uint32_t>(vertices.size_bytes())),
            m_layout
        );

        group.ibh = bgfx::createIndexBuffer(
            bgfx::copy(indices.data(), static_cast<uint32_t>(indices.size_bytes()))
        );
        if (isValidGroup(group)) {
            m_groups.push_back(group);
        } else {
            if (bgfx::isValid(group.vbh)) {
                bgfx::destroy(group.vbh);
            }
            if (bgfx::isValid(group.ibh)) {
                bgfx::destroy(group.ibh);
            }
        }
    }

    auto Mesh::loadFromBgfxGeometry(
        std::span<const std::uint8_t> bytes,
        std::size_t maximumGroups
    )
        -> std::expected<void, MeshError> {
        maximumGroups = std::min(maximumGroups, MaxMeshGroups);
        if (maximumGroups == 0) {
            return std::unexpected(MeshError{26, "Mesh group budget exhausted"});
        }
        if (bytes.empty() || bytes.size() > MaxMeshFileBytes ||
            bytes.size() > std::numeric_limits<std::uint32_t>::max()) {
            return std::unexpected(MeshError{1, "Mesh data is empty or exceeds the file limit"});
        }

        destroyHandles();

        bx::MemoryReader reader(bytes.data(), static_cast<uint32_t>(bytes.size()));
        bx::Error err;
        MeshGroup group;
        std::uint32_t groupVertexCount = 0;
        std::uint32_t groupIndexCount = 0;

        auto fail = [&](int code, std::string message) -> std::expected<void, MeshError> {
            destroyHandles();
            if (bgfx::isValid(group.vbh)) {
                bgfx::destroy(group.vbh);
            }
            if (bgfx::isValid(group.ibh)) {
                bgfx::destroy(group.ibh);
            }
            return std::unexpected(MeshError{code, std::move(message)});
        };

        uint32_t chunk = 0;
        while (reader.remaining() > 0) {
            if (reader.remaining() < static_cast<int64_t>(sizeof(chunk)) ||
                sizeof(chunk) != bx::read(&reader, chunk, &err) ||
                !err.isOk()) {
                return fail(11, "Truncated mesh chunk header");
            }
            switch (chunk) {
                case kChunkVertexBuffer: {
                    if (m_groups.size() >= maximumGroups ||
                        bgfx::isValid(group.vbh) || bgfx::isValid(group.ibh)) {
                        return fail(12, "Duplicate or out-of-order mesh vertex buffer");
                    }
                    bx::Sphere sphere;
                    bx::Aabb aabb;
                    bx::Obb obb;
                    bx::read(&reader, sphere, &err);
                    bx::read(&reader, aabb, &err);
                    bx::read(&reader, obb, &err);

                    if (!err.isOk() || !readSafeVertexLayout(reader, m_layout, err)) {
                        return fail(13, "Invalid mesh vertex layout");
                    }
                    const auto stride = m_layout.getStride();

                    uint16_t numVertices = 0;
                    bx::read(&reader, numVertices, &err);
                    if (!err.isOk() || stride == 0 || numVertices == 0) {
                        return fail(13, "Invalid mesh vertex layout or count");
                    }
                    const auto vertexBytes =
                        static_cast<std::uint64_t>(numVertices) * static_cast<std::uint64_t>(stride);
                    if (vertexBytes > MaxMeshBufferBytes ||
                        vertexBytes > static_cast<std::uint64_t>(reader.remaining())) {
                        return fail(14, "Mesh vertex buffer exceeds safe bounds");
                    }
                    const auto* mem = bgfx::copy(reader.getDataPtr(), static_cast<uint32_t>(vertexBytes));
                    bx::skip(&reader, static_cast<int32_t>(vertexBytes));
                    group.vbh = bgfx::createVertexBuffer(mem, m_layout);
                    if (!bgfx::isValid(group.vbh)) {
                        return fail(3, "Failed to create mesh vertex buffer");
                    }
                    groupVertexCount = numVertices;
                    break;
                }

                case kChunkIndexBuffer: {
                    if (!bgfx::isValid(group.vbh) || bgfx::isValid(group.ibh)) {
                        return fail(15, "Duplicate or out-of-order mesh index buffer");
                    }
                    uint32_t numIndices = 0;
                    bx::read(&reader, numIndices, &err);
                    if (!err.isOk() || numIndices == 0) {
                        return fail(16, "Invalid mesh index count");
                    }
                    const auto indexBytes =
                        static_cast<std::uint64_t>(numIndices) * sizeof(uint16_t);
                    if (indexBytes > MaxMeshBufferBytes ||
                        indexBytes > static_cast<std::uint64_t>(reader.remaining())) {
                        return fail(17, "Mesh index buffer exceeds safe bounds");
                    }
                    const auto* indexData = reader.getDataPtr();
                    for (std::uint32_t index = 0; index < numIndices; ++index) {
                        std::uint16_t vertexIndex = 0;
                        std::memcpy(
                            &vertexIndex,
                            indexData + static_cast<std::size_t>(index) * sizeof(vertexIndex),
                            sizeof(vertexIndex)
                        );
                        if (vertexIndex >= groupVertexCount) {
                            return fail(25, "Mesh index refers outside the vertex buffer");
                        }
                    }
                    const auto* mem = bgfx::copy(indexData, static_cast<uint32_t>(indexBytes));
                    bx::skip(&reader, static_cast<int32_t>(indexBytes));
                    group.ibh = bgfx::createIndexBuffer(mem);
                    if (!bgfx::isValid(group.ibh)) {
                        return fail(5, "Failed to create mesh index buffer");
                    }
                    groupIndexCount = numIndices;
                    break;
                }

                case kChunkPrimitive: {
                    if (!isValidGroup(group)) {
                        return fail(18, "Mesh primitive chunk does not have valid buffers");
                    }
                    uint16_t len = 0;
                    bx::read(&reader, len, &err);
                    if (!err.isOk() || len > reader.remaining()) {
                        return fail(19, "Invalid mesh material name length");
                    }
                    bx::skip(&reader, len);

                    uint16_t numPrimitives = 0;
                    bx::read(&reader, numPrimitives, &err);
                    if (!err.isOk()) {
                        return fail(20, "Invalid mesh primitive count");
                    }

                    for (uint32_t ii = 0; ii < numPrimitives; ++ii) {
                        bx::read(&reader, len, &err);
                        if (!err.isOk() || len > reader.remaining()) {
                            return fail(21, "Invalid mesh primitive name length");
                        }
                        bx::skip(&reader, len);

                        uint32_t startIndex = 0;
                        uint32_t numIndices = 0;
                        uint32_t startVertex = 0;
                        uint32_t numVertices = 0;
                        bx::Sphere sphere;
                        bx::Aabb aabb;
                        bx::Obb obb;
                        bx::read(&reader, startIndex, &err);
                        bx::read(&reader, numIndices, &err);
                        bx::read(&reader, startVertex, &err);
                        bx::read(&reader, numVertices, &err);
                        bx::read(&reader, sphere, &err);
                        bx::read(&reader, aabb, &err);
                        bx::read(&reader, obb, &err);
                        if (!err.isOk() ||
                            startIndex > groupIndexCount ||
                            numIndices > groupIndexCount - startIndex ||
                            startVertex > groupVertexCount ||
                            numVertices > groupVertexCount - startVertex) {
                            return fail(22, "Failed to read mesh primitive record");
                        }
                    }

                    if (m_groups.size() >= maximumGroups) {
                        return fail(23, "Mesh group limit exceeded");
                    }

                    m_groups.push_back(group);
                    group = {};
                    groupVertexCount = 0;
                    groupIndexCount = 0;
                    break;
                }

                case kChunkVertexBufferCompressed:
                case kChunkIndexBufferCompressed:
                    return fail(8, "Compressed bgfx geometry is not supported yet");

                default:
                    return fail(9, "Unknown bgfx geometry chunk");
            }
        }

        if (isValidGroup(group)) {
            if (m_groups.size() >= maximumGroups) {
                return fail(23, "Mesh group limit exceeded");
            }
            m_groups.push_back(group);
            group = {};
        } else if (bgfx::isValid(group.vbh) || bgfx::isValid(group.ibh)) {
            return fail(24, "Mesh ended with an incomplete buffer group");
        }

        if (m_groups.empty()) {
            return fail(10, "Mesh file did not contain renderable groups");
        }

        return {};
    }

    auto Mesh::submit(
        bgfx::ProgramHandle program,
        bgfx::UniformHandle colorUniform,
        bgfx::UniformHandle samplerUniform,
        bgfx::TextureHandle texture,
        const Transform &transform,
        math::Color baseColor,
        bgfx::ViewId viewId,
        uint64_t stage,
        std::size_t maximumGroups
    ) const -> std::size_t {
        if (m_groups.empty() || maximumGroups == 0 ||
            !bgfx::isValid(program) || !bgfx::isValid(texture)) {
            return 0;
        }

        const auto model = math::Mat4::fromTransform(transform);
        std::size_t submitted = 0;

        for (const auto& group : m_groups) {
            if (submitted >= maximumGroups) {
                break;
            }
            if (!isValidGroup(group)) {
                continue;
            }

            bgfx::setTransform(model.data());
            bgfx::setState(stage);
            bgfx::setVertexBuffer(0, group.vbh);

            if (bgfx::isValid(group.ibh)) {
                bgfx::setIndexBuffer(group.ibh);
            }
            float color[4] = {baseColor.r, baseColor.g, baseColor.b, baseColor.a};
            bgfx::setUniform(colorUniform, color, 1);
            bgfx::setTexture(0, samplerUniform, texture);
            bgfx::submit(viewId, program);
            ++submitted;
        }
        return submitted;
    }

    auto Mesh::isValid() const -> bool {
        return std::ranges::any_of(m_groups, [](const auto& group) { return isValidGroup(group); });
    }
    auto destroy_if_valid(auto& handle) {
        if (bgfx::isValid(handle)) {
            bgfx::destroy(handle);
            handle = BGFX_INVALID_HANDLE;
        }
    }
    auto Mesh::destroyHandles() -> void {
        std::ranges::for_each(m_groups, [&](auto& group) {
            destroy_if_valid(group.vbh);
            destroy_if_valid(group.ibh);
        });
        m_groups.clear();
    }

    Mesh::~Mesh() {
        destroyHandles();
    }
} // graphics
