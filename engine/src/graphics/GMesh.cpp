//
// Created by drhaz on 20.06.2026.
//

#include "graphics/GMesh.hpp"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "math/Detail.hpp"

namespace bgfx {
    int32_t read(bx::ReaderI* reader, bgfx::VertexLayout& layout, bx::Error* err);
}

namespace {
    constexpr uint32_t kChunkVertexBuffer = BX_MAKEFOURCC('V', 'B', ' ', 0x1);
    constexpr uint32_t kChunkVertexBufferCompressed = BX_MAKEFOURCC('V', 'B', 'C', 0x0);
    constexpr uint32_t kChunkIndexBuffer = BX_MAKEFOURCC('I', 'B', ' ', 0x0);
    constexpr uint32_t kChunkIndexBufferCompressed = BX_MAKEFOURCC('I', 'B', 'C', 0x1);
    constexpr uint32_t kChunkPrimitive = BX_MAKEFOURCC('P', 'R', 'I', 0x0);

    auto readFileBytes(std::string_view filename) -> std::vector<uint8_t> {
        std::ifstream file(std::string{filename}, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return {};
        }

        const auto size = file.tellg();
        if (size <= 0) {
            return {};
        }

        std::vector<uint8_t> bytes(static_cast<std::size_t>(size));
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(bytes.data()), size);
        if (!file) {
            return {};
        }

        return bytes;
    }

    auto isValidGroup(const engine::graphics::GMeshGroup& group) -> bool {
        return bgfx::isValid(group.vbh) && bgfx::isValid(group.ibh);
    }
}


namespace engine::graphics {

    GMesh::GMesh(GMesh &&other) noexcept {
        this->m_groups = std::move(other.m_groups);
        this->m_layout = std::move(other.m_layout);

        other.m_groups.clear();
        other.m_layout = {};
    }

    GMesh & GMesh::operator=(GMesh &&other) noexcept {
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

    void GMesh::createTriangle() {
        static const GVertex vertices[3] = {
            {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f},
            { 0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f},
            { 0.0f,  0.5f, 0.0f, 0.0f, 0.0f, -1.0f}
        };

        static const uint16_t indices[3] = { 0, 2, 1 };

        destroyHandles();

        m_layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal,   3, bgfx::AttribType::Float)
            .end();

        GMeshGroup group;
        group.vbh = bgfx::createVertexBuffer(
            bgfx::makeRef(vertices, sizeof(vertices)),
            m_layout
        );

        group.ibh = bgfx::createIndexBuffer(
            bgfx::makeRef(indices, sizeof(indices))
        );
        if (isValidGroup(group)) {
            m_groups.push_back(group);
        }
    }

    void GMesh::createFromVertices(std::span<const GVertex> vertices, std::span<const uint16_t> indices) {
        destroyHandles();

        m_layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal,   3, bgfx::AttribType::Float)
            .end();

        GMeshGroup group;
        group.vbh = bgfx::createVertexBuffer(
            bgfx::copy(vertices.data(), static_cast<uint32_t>(vertices.size_bytes())),
            m_layout
        );

        group.ibh = bgfx::createIndexBuffer(
            bgfx::copy(indices.data(), static_cast<uint32_t>(indices.size_bytes()))
        );
        if (isValidGroup(group)) {
            m_groups.push_back(group);
        }
    }

    auto GMesh::loadFromBgfxGeometry(std::string_view filename) -> std::expected<void, GMeshError> {
        auto bytes = readFileBytes(filename);
        if (bytes.empty()) {
            return std::unexpected(GMeshError{1, "Failed to read mesh file"});
        }

        destroyHandles();

        bx::MemoryReader reader(bytes.data(), static_cast<uint32_t>(bytes.size()));
        bx::Error err;
        GMeshGroup group;

        auto fail = [&](int code, std::string message) -> std::expected<void, GMeshError> {
            destroyHandles();
            if (bgfx::isValid(group.vbh)) {
                bgfx::destroy(group.vbh);
            }
            if (bgfx::isValid(group.ibh)) {
                bgfx::destroy(group.ibh);
            }
            return std::unexpected(GMeshError{code, std::move(message)});
        };

        uint32_t chunk = 0;
        while (4 == bx::read(&reader, chunk, &err) && err.isOk()) {
            switch (chunk) {
                case kChunkVertexBuffer: {
                    bx::Sphere sphere;
                    bx::Aabb aabb;
                    bx::Obb obb;
                    bx::read(&reader, sphere, &err);
                    bx::read(&reader, aabb, &err);
                    bx::read(&reader, obb, &err);

                    bgfx::read(&reader, m_layout, &err);
                    const auto stride = m_layout.getStride();

                    uint16_t numVertices = 0;
                    bx::read(&reader, numVertices, &err);
                    const bgfx::Memory* mem = bgfx::alloc(numVertices * stride);
                    bx::read(&reader, mem->data, mem->size, &err);

                    if (!err.isOk()) {
                        return fail(2, "Failed to read mesh vertex buffer");
                    }

                    group.vbh = bgfx::createVertexBuffer(mem, m_layout);
                    if (!bgfx::isValid(group.vbh)) {
                        return fail(3, "Failed to create mesh vertex buffer");
                    }
                    break;
                }

                case kChunkIndexBuffer: {
                    uint32_t numIndices = 0;
                    bx::read(&reader, numIndices, &err);

                    const bgfx::Memory* mem = bgfx::alloc(numIndices * sizeof(uint16_t));
                    bx::read(&reader, mem->data, mem->size, &err);

                    if (!err.isOk()) {
                        return fail(4, "Failed to read mesh index buffer");
                    }

                    group.ibh = bgfx::createIndexBuffer(mem);
                    if (!bgfx::isValid(group.ibh)) {
                        return fail(5, "Failed to create mesh index buffer");
                    }
                    break;
                }

                case kChunkPrimitive: {
                    uint16_t len = 0;
                    bx::read(&reader, len, &err);
                    bx::skip(&reader, len);

                    uint16_t numPrimitives = 0;
                    bx::read(&reader, numPrimitives, &err);

                    for (uint32_t ii = 0; ii < numPrimitives; ++ii) {
                        bx::read(&reader, len, &err);
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
                    }

                    if (!err.isOk()) {
                        return fail(6, "Failed to read mesh primitive chunk");
                    }
                    if (!isValidGroup(group)) {
                        return fail(7, "Mesh primitive chunk does not have valid buffers");
                    }

                    m_groups.push_back(group);
                    group = {};
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
            m_groups.push_back(group);
        }

        if (m_groups.empty()) {
            return fail(10, "Mesh file did not contain renderable groups");
        }

        return {};
    }

    void GMesh::submit(bgfx::ProgramHandle program, const GTransform &transform, uint8_t viewId) const {
        if (m_groups.empty() || !bgfx::isValid(program)) {
            return;
        }

        const auto position = math::detail::toGlm(transform.position);
        const auto rotation = math::detail::toGlm(transform.rotation);
        const auto scale = math::detail::toGlm(transform.scale);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), position)
                        * glm::mat4_cast(glm::normalize(rotation))
                        * glm::scale(glm::mat4(1.0f), scale);


        for (const auto& group : m_groups) {
            if (!isValidGroup(group)) {
                continue;
            }

            bgfx::setTransform(glm::value_ptr(model));
            bgfx::setState(BGFX_STATE_WRITE_RGB |
                BGFX_STATE_WRITE_A |
                BGFX_STATE_WRITE_Z |
                BGFX_STATE_DEPTH_TEST_LESS |
                BGFX_STATE_MSAA
            );

            bgfx::setVertexBuffer(0, group.vbh);
            if (bgfx::isValid(group.ibh)) {
                bgfx::setIndexBuffer(group.ibh);
            }

            bgfx::submit(viewId, program);
        }
    }

    auto GMesh::isValid() const -> bool {
        return std::ranges::any_of(m_groups, [](const auto& group) { return !isValidGroup(group); });
    }

    auto GMesh::destroyHandles() -> void {
        for (auto& group : m_groups) {
            if (bgfx::isValid(group.vbh)) {
                bgfx::destroy(group.vbh);
                group.vbh = BGFX_INVALID_HANDLE;
            }
            if (bgfx::isValid(group.ibh)) {
                bgfx::destroy(group.ibh);
                group.ibh = BGFX_INVALID_HANDLE;
            }
        }
        m_groups.clear();
    }

    GMesh::~GMesh() {
        destroyHandles();
    }
} // graphics
