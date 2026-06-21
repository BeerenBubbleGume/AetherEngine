//
// Created by drhaz on 20.06.2026.
//

#include "graphics/GMesh.hpp"


namespace engine::graphics {

    GMesh::GMesh(GMesh &&other) noexcept {
        this->m_vbh = std::move(other.m_vbh);
        this->m_ibh = std::move(other.m_ibh);
        this->m_layout = std::move(other.m_layout);

        other.m_vbh = BGFX_INVALID_HANDLE;
        other.m_ibh = BGFX_INVALID_HANDLE;
        other.m_layout = {};
    }

    GMesh & GMesh::operator=(GMesh &&other) noexcept {
        this->m_vbh = std::move(other.m_vbh);
        this->m_ibh = std::move(other.m_ibh);
        this->m_layout = std::move(other.m_layout);

        other.m_vbh = BGFX_INVALID_HANDLE;
        other.m_ibh = BGFX_INVALID_HANDLE;
        other.m_layout = {};
        return *this;
    }

    void GMesh::createTriangle() {
        static const PosColorVertex vertices[3] = {
            {-0.5f, -0.5f, 0.0f, 0xff0000ff}, // красный
            { 0.5f, -0.5f, 0.0f, 0xff00ff00}, // зелёный
            { 0.0f,  0.5f, 0.0f, 0xffff0000}  // синий
        };

        static const uint16_t indices[3] = { 0, 2, 1 };

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

    void GMesh::submit(bgfx::ProgramHandle program, const GTransform &transform, uint8_t viewId) const {
        if (!bgfx::isValid(m_vbh) || !bgfx::isValid(program)) {
            return;
        }

        float mtx[16];
        bx::mtxSRT(mtx,
            transform.scale.x, transform.scale.y, transform.scale.z,   // scale
            bx::toRad(transform.rotation.x),
            bx::toRad(transform.rotation.y),
            bx::toRad(transform.rotation.z),   // rotation (в радианах)
            transform.position.x,
            transform.position.y,
            transform.position.z                   // position
        );

        bgfx::setTransform(mtx);

        // Геометрия
        bgfx::setVertexBuffer(0, m_vbh);
        if (bgfx::isValid(m_ibh)) {
            bgfx::setIndexBuffer(m_ibh);
        }

        // Отправляем на рендер
        bgfx::submit(viewId, program);
    }

    auto GMesh::isValid() const -> bool {
        return bgfx::isValid(m_vbh) && bgfx::isValid(m_ibh);
    }

    GMesh::~GMesh() {
        if (bgfx::isValid(m_vbh)) {
            bgfx::destroy(m_vbh);
        }
        if (bgfx::isValid(m_ibh)) {
            bgfx::destroy(m_ibh);
        }
    }
} // graphics