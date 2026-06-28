//
// Created by drhaz on 25.06.2026.
//

#include "math/UTypes.hpp"

#include "math/Detail.hpp"

#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


namespace engine::math {
        namespace {
                constexpr float kPi = 3.14159265358979323846f;

                float toRadians(float degrees) {
                        return degrees * kPi / 180.0f;
                }
        }

        TVec3 & TVec3::operator+=(const TVec3 &other) {
                x += other.x;
                y += other.y;
                z += other.z;
                return *this;
        }

        TVec3 & TVec3::operator-=(const TVec3 &other) {
                x -= other.x;
                y -= other.y;
                z -= other.z;
                return *this;
        }

        TVec3 & TVec3::operator*=(const TVec3 &other) {
                x *= other.x;
                y *= other.y;
                z *= other.z;
                return *this;
        }

        TVec3 & TVec3::operator/=(const TVec3 &other) {
                x /= other.x;
                y /= other.y;
                z /= other.z;
                return *this;
        }

        TVec3 & TVec3::operator*=(float scalar) {
                x *= scalar;
                y *= scalar;
                z *= scalar;
                return *this;
        }

        TVec3 & TVec3::operator/=(float scalar) {
                x /= scalar;
                y /= scalar;
                z /= scalar;
                return *this;
        }

        TVec3 TVec3::operator+(const TVec3 &other) const {
                return {x + other.x, y + other.y, z + other.z};
        }

        TVec3 TVec3::operator-(const TVec3 &other) const {
                return {x - other.x, y - other.y, z - other.z};
        }

        TVec3 TVec3::operator*(const TVec3 &other) const {
                return {x * other.x, y * other.y, z * other.z};
        }

        TVec3 TVec3::operator/(const TVec3 &other) const {
                return {x / other.x, y / other.y, z / other.z};
        }

        TVec3 TVec3::operator*(float scalar) const {
                return {x * scalar, y * scalar, z * scalar};
        }

        TVec3 TVec3::operator/(float scalar) const {
                return {x / scalar, y / scalar, z / scalar};
        }

        TVec3 TVec3::operator-() const {
                return {-x, -y, -z};
        }

        TVec3 & TVec3::operator=(const TVec3 &other) {
                if (this == &other) {
                        return *this;
                }
                x = other.x;
                y = other.y;
                z = other.z;
                return *this;
        }

        TVec3 & TVec3::operator=(TVec3 &&other) noexcept {
                if (this == &other) {
                        return *this;
                }
                x = other.x;
                y = other.y;
                z = other.z;
                return *this;
        }

        bool TVec3::operator==(const TVec3 &other) const {
                return x == other.x && y == other.y && z == other.z;
        }

        bool TVec3::operator!=(const TVec3 &other) const {
                return !(*this == other);
        }

        float TVec3::dot(const TVec3 &other) const {
                return x * other.x + y * other.y + z * other.z;
        }

        TVec3 TVec3::cross(const TVec3 &other) const {
                return {
                        y * other.z - z * other.y,
                        z * other.x - x * other.z,
                        x * other.y - y * other.x
                };
        }

        float TVec3::length() const {
                return std::sqrt(dot(*this));
        }

        TVec3 TVec3::normalized() const {
                const float len = length();
                if (len <= 0.0f) {
                        return zero();
                }
                return *this / len;
        }

        TVec3 operator*(float scalar, const TVec3& vec) {
                return vec * scalar;
        }

        TVec2 & TVec2::operator+=(const TVec2 &other) {
                x += other.x;
                y += other.y;
                return *this;
        }

        TVec2 & TVec2::operator-=(const TVec2 &other) {
                x -= other.x;
                y -= other.y;
                return *this;
        }

        TVec2 & TVec2::operator*=(const TVec2 &other) {
                x *= other.x;
                y *= other.y;
                return *this;
        }

        TVec2 & TVec2::operator/=(const TVec2 &other) {
                x /= other.x;
                y /= other.y;
                return *this;
        }

        TVec2 & TVec2::operator*=(float scalar) {
                x *= scalar;
                y *= scalar;
                return *this;
        }

        TVec2 & TVec2::operator/=(float scalar) {
                x /= scalar;
                y /= scalar;
                return *this;
        }

        TVec2 TVec2::operator+(const TVec2 &other) const {
                return {x + other.x, y + other.y};
        }

        TVec2 TVec2::operator-(const TVec2 &other) const {
                return {x - other.x, y - other.y};
        }

        TVec2 TVec2::operator*(const TVec2 &other) const {
                return {x * other.x, y * other.y};
        }

        TVec2 TVec2::operator/(const TVec2 &other) const {
                return {x / other.x, y / other.y};
        }

        TVec2 TVec2::operator*(float scalar) const {
                return {x * scalar, y * scalar};
        }

        TVec2 TVec2::operator/(float scalar) const {
                return {x / scalar, y / scalar};
        }

        TVec2 TVec2::operator-() const {
                return {-x, -y};
        }

        TVec2 & TVec2::operator=(const TVec2 &other) {
                if (this == &other) {
                        return *this;
                }
                x = other.x;
                y = other.y;
                return *this;
        }

        TVec2 & TVec2::operator=(TVec2 &&other) noexcept {
                if (this == &other) {
                        return *this;
                }
                x = other.x;
                y = other.y;
                return *this;
        }

        bool TVec2::operator==(const TVec2 &other) const {
                return x == other.x && y == other.y;
        }

        bool TVec2::operator!=(const TVec2 &other) const {
                return !(*this == other);
        }

        float TVec2::dot(const TVec2 &other) const {
                return x * other.x + y * other.y;
        }

        float TVec2::length() const {
                return std::sqrt(dot(*this));
        }

        TVec2 TVec2::normalized() const {
                const float len = length();
                if (len <= 0.0f) {
                        return zero();
                }
                return *this / len;
        }

        TVec2 operator*(float scalar, const TVec2& vec) {
                return vec * scalar;
        }

        TQuat & TQuat::operator*=(const TQuat &other) {
                *this = *this * other;
                return *this;
        }

        TQuat TQuat::operator*(const TQuat &other) const {
                return {
                        w * other.w - x * other.x - y * other.y - z * other.z,
                        w * other.x + x * other.w + y * other.z - z * other.y,
                        w * other.y - x * other.z + y * other.w + z * other.x,
                        w * other.z + x * other.y - y * other.x + z * other.w
                };
        }

        TQuat & TQuat::operator=(const TQuat &other) {
                if (this == &other) {
                        return *this;
                }
                w = other.w;
                x = other.x;
                y = other.y;
                z = other.z;
                return *this;
        }

        TQuat & TQuat::operator=(TQuat &&other) noexcept {
                if (this == &other) {
                        return *this;
                }
                w = other.w;
                x = other.x;
                y = other.y;
                z = other.z;
                return *this;
        }

        bool TQuat::operator==(const TQuat &other) const {
                return w == other.w && x == other.x && y == other.y && z == other.z;
        }

        bool TQuat::operator!=(const TQuat &other) const {
                return !(*this == other);
        }

        float TQuat::length() const {
                return std::sqrt(w * w + x * x + y * y + z * z);
        }

        TQuat TQuat::normalized() const {
                const float len = length();
                if (len <= 0.0f) {
                        return identity();
                }
                return {w / len, x / len, y / len, z / len};
        }

        TVec3 TQuat::rotate(TVec3 vector) const {
                const TQuat q = normalized();
                const TVec3 u{q.x, q.y, q.z};
                const float s = q.w;

                return (2.0f * u.dot(vector)) * u +
                       (s * s - u.dot(u)) * vector +
                       (2.0f * s) * u.cross(vector);
        }

        TQuat TQuat::identity() {
                return {1.0f, 0.0f, 0.0f, 0.0f};
        }

        TQuat TQuat::fromAxisAngleDegrees(TVec3 axis, float degrees) {
                return fromAxisAngleRadians(axis, toRadians(degrees));
        }

        TQuat TQuat::fromAxisAngleRadians(TVec3 axis, float radians) {
                const TVec3 normalizedAxis = axis.normalized();
                if (normalizedAxis == TVec3::zero()) {
                        return identity();
                }

                const float halfAngle = radians * 0.5f;
                const float sinHalfAngle = std::sin(halfAngle);

                return {
                        std::cos(halfAngle),
                        normalizedAxis.x * sinHalfAngle,
                        normalizedAxis.y * sinHalfAngle,
                        normalizedAxis.z * sinHalfAngle
                };
        }

        TQuat TQuat::fromEulerXYZDegrees(TVec3 pitchYawRoll) {
                return fromEulerXYZRadians({
                        toRadians(pitchYawRoll.x),
                        toRadians(pitchYawRoll.y),
                        toRadians(pitchYawRoll.z)
                });
        }

        TQuat TQuat::fromEulerXYZRadians(TVec3 pitchYawRoll) {
                const TVec3 halfAngles = pitchYawRoll * 0.5f;

                const TVec3 c{
                        std::cos(halfAngles.x),
                        std::cos(halfAngles.y),
                        std::cos(halfAngles.z)
                };
                const TVec3 s{
                        std::sin(halfAngles.x),
                        std::sin(halfAngles.y),
                        std::sin(halfAngles.z)
                };

                return {
                        c.x * c.y * c.z + s.x * s.y * s.z,
                        s.x * c.y * c.z - c.x * s.y * s.z,
                        c.x * s.y * c.z + s.x * c.y * s.z,
                        c.x * c.y * s.z - s.x * s.y * c.z
                };
        }

        TMat4::TMat4() : TMat4(1.0f) {
        }

        TMat4::TMat4(float diagonal) : m{} {
                m[0] = diagonal;
                m[5] = diagonal;
                m[10] = diagonal;
                m[15] = diagonal;
        }

        TMat4::TMat4(std::array<float, 16> values) : m(values) {
        }

        float* TMat4::data() {
                return m.data();
        }

        const float* TMat4::data() const {
                return m.data();
        }

        float& TMat4::operator[](std::size_t index) {
                return m[index];
        }

        const float& TMat4::operator[](std::size_t index) const {
                return m[index];
        }

        TMat4& TMat4::operator*=(const TMat4& other) {
                *this = *this * other;
                return *this;
        }

        TMat4 TMat4::operator*(const TMat4& other) const {
                return detail::fromGlm(detail::toGlm(*this) * detail::toGlm(other));
        }

        bool TMat4::operator==(const TMat4& other) const {
                return m == other.m;
        }

        bool TMat4::operator!=(const TMat4& other) const {
                return !(*this == other);
        }

        TMat4 TMat4::zero() {
                return TMat4{std::array<float, 16>{}};
        }

        TMat4 TMat4::identity() {
                return TMat4{1.0f};
        }

        TMat4 TMat4::translation(TVec3 translation) {
                return detail::fromGlm(glm::translate(glm::mat4{1.0f}, detail::toGlm(translation)));
        }

        TMat4 TMat4::rotation(TQuat rotation) {
                return detail::fromGlm(glm::mat4_cast(detail::toGlm(rotation.normalized())));
        }

        TMat4 TMat4::scale(TVec3 scale) {
                return detail::fromGlm(glm::scale(glm::mat4{1.0f}, detail::toGlm(scale)));
        }

        TMat4 TMat4::fromTransform(const Transform& transform) {
                return translation(transform.position) *
                       rotation(transform.rotation) *
                       scale(transform.scale);
        }

        TMat4 TMat4::lookAtLeftHanded(TVec3 eye, TVec3 at, TVec3 up) {
                return detail::fromGlm(glm::lookAtLH(
                        detail::toGlm(eye),
                        detail::toGlm(at),
                        detail::toGlm(up)
                ));
        }

        void Transform::reset() {
                position = TVec3::zero();
                rotation = TQuat::identity();
                scale = TVec3::one();
        }

        void Transform::setPosition(TVec3 pos) {
                position = pos;
        }

        void Transform::setRotation(TQuat rot) {
                rotation = rot;
        }

        void Transform::setScale(TVec3 newScale) {
                scale = newScale;
        }
} // namespace engine::math
