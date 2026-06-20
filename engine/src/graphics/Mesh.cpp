//
// Created by drhaz on 20.06.2026.
//

#include "graphics/Mesh.hpp"


namespace engine::graphics {
    GMesh::GMeshPtr GMesh::createMesh() {
        return GMeshPtr(new GMesh(), GMeshDeleter{});
    }

    GMesh::GMesh(GMesh &&other) noexcept {
        this->m_vbh = std::move(other.m_vbh);
        this->m_ibh = std::move(other.m_ibh);
        this->m_layout = std::move(other.m_layout);
    }

    GMesh & GMesh::operator=(GMesh &&other) noexcept {
        this->m_vbh = std::move(other.m_vbh);
        this->m_ibh = std::move(other.m_ibh);
        this->m_layout = std::move(other.m_layout);
        return *this;
    }

    void GMesh::createTriangle() {
        static const PosColorVertex vertices[3] = {
            {-0.5f, -0.5f, 0.0f, 0xff0000ff}, // красный
            { 0.5f, -0.5f, 0.0f, 0xff00ff00}, // зелёный
            { 0.0f,  0.5f, 0.0f, 0xffff0000}  // синий
        };

        static const uint16_t indices[3] = { 0, 1, 2 };

        m_layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
            .end();

        m_vbh = bgfx::createVertexBuffer(
            bgfx::makeRef(vertices, sizeof(vertices)),
            m_layout
        );

        m_ibh = bgfx::createIndexBuffer(
            bgfx::makeRef(indices, sizeof(indices))
        );
    }

    void GMesh::submit(bgfx::ProgramHandle program) const {
        if (!bgfx::isValid(m_vbh) || !bgfx::isValid(program)) {
            return;
        }

        // Устанавливаем геометрию
        bgfx::setVertexBuffer(0, m_vbh);

        if (bgfx::isValid(m_ibh)) {
            bgfx::setIndexBuffer(m_ibh);
        }

        // Можно добавить трансформации, uniform'ы и т.д. позже
        // bgfx::setTransform(...);

        // Отправляем на рендер
        bgfx::submit(0, program);
    }

    auto GMesh::isValid() const -> bool {
        return bgfx::isValid(m_vbh) && bgfx::isValid(m_ibh);
    }
} // graphics