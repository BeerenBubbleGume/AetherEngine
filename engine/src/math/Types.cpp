//
// Created by drhaz on 25.06.2026.
//

#include "math/Types.hpp"

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

        Vec3 & Vec3::operator+=(const Vec3 &other) {
                x += other.x;
                y += other.y;
                z += other.z;
                return *this;
        }

        Vec3 & Vec3::operator-=(const Vec3 &other) {
                x -= other.x;
                y -= other.y;
                z -= other.z;
                return *this;
        }

        Vec3 & Vec3::operator*=(const Vec3 &other) {
                x *= other.x;
                y *= other.y;
                z *= other.z;
                return *this;
        }

        Vec3 & Vec3::operator/=(const Vec3 &other) {
                x /= other.x;
                y /= other.y;
                z /= other.z;
                return *this;
        }

        Vec3 & Vec3::operator*=(float scalar) {
                x *= scalar;
                y *= scalar;
                z *= scalar;
                return *this;
        }

        Vec3 & Vec3::operator/=(float scalar) {
                x /= scalar;
                y /= scalar;
                z /= scalar;
                return *this;
        }

        Vec3 Vec3::operator+(const Vec3 &other) const {
                return {x + other.x, y + other.y, z + other.z};
        }

        Vec3 Vec3::operator-(const Vec3 &other) const {
                return {x - other.x, y - other.y, z - other.z};
        }

        Vec3 Vec3::operator*(const Vec3 &other) const {
                return {x * other.x, y * other.y, z * other.z};
        }

        Vec3 Vec3::operator/(const Vec3 &other) const {
                return {x / other.x, y / other.y, z / other.z};
        }

        Vec3 Vec3::operator*(float scalar) const {
                return {x * scalar, y * scalar, z * scalar};
        }

        Vec3 Vec3::operator/(float scalar) const {
                return {x / scalar, y / scalar, z / scalar};
        }

        Vec3 Vec3::operator-() const {
                return {-x, -y, -z};
        }

        Vec3 & Vec3::operator=(const Vec3 &other) {
                if (this == &other) {
                        return *this;
                }
                x = other.x;
                y = other.y;
                z = other.z;
                return *this;
        }

        Vec3 & Vec3::operator=(Vec3 &&other) noexcept {
                if (this == &other) {
                        return *this;
                }
                x = other.x;
                y = other.y;
                z = other.z;
                return *this;
        }

        bool Vec3::operator==(const Vec3 &other) const {
                return x == other.x && y == other.y && z == other.z;
        }

        bool Vec3::operator!=(const Vec3 &other) const {
                return !(*this == other);
        }

        float Vec3::dot(const Vec3 &other) const {
                return x * other.x + y * other.y + z * other.z;
        }

        Vec3 Vec3::cross(const Vec3 &other) const {
                return {
                        y * other.z - z * other.y,
                        z * other.x - x * other.z,
                        x * other.y - y * other.x
                };
        }

        float Vec3::length() const {
                return std::sqrt(dot(*this));
        }

        Vec3 Vec3::normalized() const {
                const float len = length();
                if (len <= 0.0f) {
                        return zero();
                }
                return *this / len;
        }

        Vec3 operator*(float scalar, const Vec3& vec) {
                return vec * scalar;
        }

        Vec2 & Vec2::operator+=(const Vec2 &other) {
                x += other.x;
                y += other.y;
                return *this;
        }

        Vec2 & Vec2::operator-=(const Vec2 &other) {
                x -= other.x;
                y -= other.y;
                return *this;
        }

        Vec2 & Vec2::operator*=(const Vec2 &other) {
                x *= other.x;
                y *= other.y;
                return *this;
        }

        Vec2 & Vec2::operator/=(const Vec2 &other) {
                x /= other.x;
                y /= other.y;
                return *this;
        }

        Vec2 & Vec2::operator*=(float scalar) {
                x *= scalar;
                y *= scalar;
                return *this;
        }

        Vec2 & Vec2::operator/=(float scalar) {
                x /= scalar;
                y /= scalar;
                return *this;
        }

        Vec2 Vec2::operator+(const Vec2 &other) const {
                return {x + other.x, y + other.y};
        }

        Vec2 Vec2::operator-(const Vec2 &other) const {
                return {x - other.x, y - other.y};
        }

        Vec2 Vec2::operator*(const Vec2 &other) const {
                return {x * other.x, y * other.y};
        }

        Vec2 Vec2::operator/(const Vec2 &other) const {
                return {x / other.x, y / other.y};
        }

        Vec2 Vec2::operator*(float scalar) const {
                return {x * scalar, y * scalar};
        }

        Vec2 Vec2::operator/(float scalar) const {
                return {x / scalar, y / scalar};
        }

        Vec2 Vec2::operator-() const {
                return {-x, -y};
        }

        Vec2 & Vec2::operator=(const Vec2 &other) {
                if (this == &other) {
                        return *this;
                }
                x = other.x;
                y = other.y;
                return *this;
        }

        Vec2 & Vec2::operator=(Vec2 &&other) noexcept {
                if (this == &other) {
                        return *this;
                }
                x = other.x;
                y = other.y;
                return *this;
        }

        bool Vec2::operator==(const Vec2 &other) const {
                return x == other.x && y == other.y;
        }

        bool Vec2::operator!=(const Vec2 &other) const {
                return !(*this == other);
        }

        float Vec2::dot(const Vec2 &other) const {
                return x * other.x + y * other.y;
        }

        float Vec2::length() const {
                return std::sqrt(dot(*this));
        }

        Vec2 Vec2::normalized() const {
                const float len = length();
                if (len <= 0.0f) {
                        return zero();
                }
                return *this / len;
        }

        Vec2 operator*(float scalar, const Vec2& vec) {
                return vec * scalar;
        }

        Quat & Quat::operator*=(const Quat &other) {
                *this = *this * other;
                return *this;
        }

        Quat Quat::operator*(const Quat &other) const {
                return {
                        w * other.w - x * other.x - y * other.y - z * other.z,
                        w * other.x + x * other.w + y * other.z - z * other.y,
                        w * other.y - x * other.z + y * other.w + z * other.x,
                        w * other.z + x * other.y - y * other.x + z * other.w
                };
        }

        Quat & Quat::operator=(const Quat &other) {
                if (this == &other) {
                        return *this;
                }
                w = other.w;
                x = other.x;
                y = other.y;
                z = other.z;
                return *this;
        }

        Quat & Quat::operator=(Quat &&other) noexcept {
                if (this == &other) {
                        return *this;
                }
                w = other.w;
                x = other.x;
                y = other.y;
                z = other.z;
                return *this;
        }

        bool Quat::operator==(const Quat &other) const {
                return w == other.w && x == other.x && y == other.y && z == other.z;
        }

        bool Quat::operator!=(const Quat &other) const {
                return !(*this == other);
        }

        float Quat::length() const {
                return std::sqrt(w * w + x * x + y * y + z * z);
        }

        Quat Quat::normalized() const {
                const float len = length();
                if (len <= 0.0f) {
                        return identity();
                }
                return {w / len, x / len, y / len, z / len};
        }

        Vec3 Quat::rotate(Vec3 vector) const {
                const Quat q = normalized();
                const Vec3 u{q.x, q.y, q.z};
                const float s = q.w;

                return (2.0f * u.dot(vector)) * u +
                       (s * s - u.dot(u)) * vector +
                       (2.0f * s) * u.cross(vector);
        }

        Quat Quat::identity() {
                return {1.0f, 0.0f, 0.0f, 0.0f};
        }

        Quat Quat::fromAxisAngleDegrees(Vec3 axis, float degrees) {
                return fromAxisAngleRadians(axis, toRadians(degrees));
        }

        Quat Quat::fromAxisAngleRadians(Vec3 axis, float radians) {
                const Vec3 normalizedAxis = axis.normalized();
                if (normalizedAxis == Vec3::zero()) {
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

        Quat Quat::fromEulerXYZDegrees(Vec3 pitchYawRoll) {
                return fromEulerXYZRadians({
                        toRadians(pitchYawRoll.x),
                        toRadians(pitchYawRoll.y),
                        toRadians(pitchYawRoll.z)
                });
        }

        Quat Quat::fromEulerXYZRadians(Vec3 pitchYawRoll) {
                const Vec3 halfAngles = pitchYawRoll * 0.5f;

                const Vec3 c{
                        std::cos(halfAngles.x),
                        std::cos(halfAngles.y),
                        std::cos(halfAngles.z)
                };
                const Vec3 s{
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

        Mat4::Mat4() : Mat4(1.0f) {
        }

        Mat4::Mat4(float diagonal) : m{} {
                m[0] = diagonal;
                m[5] = diagonal;
                m[10] = diagonal;
                m[15] = diagonal;
        }

        Mat4::Mat4(std::array<float, 16> values) : m(values) {
        }

        float* Mat4::data() {
                return m.data();
        }

        const float* Mat4::data() const {
                return m.data();
        }

        float& Mat4::operator[](std::size_t index) {
                return m[index];
        }

        const float& Mat4::operator[](std::size_t index) const {
                return m[index];
        }

        Mat4& Mat4::operator*=(const Mat4& other) {
                *this = *this * other;
                return *this;
        }

        Mat4 Mat4::operator*(const Mat4& other) const {
                return detail::fromGlm(detail::toGlm(*this) * detail::toGlm(other));
        }

        bool Mat4::operator==(const Mat4& other) const {
                return m == other.m;
        }

        bool Mat4::operator!=(const Mat4& other) const {
                return !(*this == other);
        }

        Mat4 Mat4::zero() {
                return Mat4{std::array<float, 16>{}};
        }

        Mat4 Mat4::identity() {
                return Mat4{1.0f};
        }

        Mat4 Mat4::translation(Vec3 translation) {
                return detail::fromGlm(glm::translate(glm::mat4{1.0f}, detail::toGlm(translation)));
        }

        Mat4 Mat4::rotation(Quat rotation) {
                return detail::fromGlm(glm::mat4_cast(detail::toGlm(rotation.normalized())));
        }

        Mat4 Mat4::scale(Vec3 scale) {
                return detail::fromGlm(glm::scale(glm::mat4{1.0f}, detail::toGlm(scale)));
        }

        Mat4 Mat4::fromTransform(const Transform& transform) {
                return translation(transform.position) *
                       rotation(transform.rotation) *
                       scale(transform.scale);
        }

        Mat4 Mat4::lookAtLeftHanded(Vec3 eye, Vec3 at, Vec3 up) {
                return detail::fromGlm(glm::lookAtLH(
                        detail::toGlm(eye),
                        detail::toGlm(at),
                        detail::toGlm(up)
                ));
        }

        void Transform::reset() {
                position = Vec3::zero();
                rotation = Quat::identity();
                scale = Vec3::one();
        }

        void Transform::setPosition(Vec3 pos) {
                position = pos;
        }

        void Transform::setRotation(Quat rot) {
                rotation = rot;
        }

        void Transform::setScale(Vec3 newScale) {
                scale = newScale;
        }
} // namespace engine::math
