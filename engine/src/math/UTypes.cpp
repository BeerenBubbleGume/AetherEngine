//
// Created by drhaz on 25.06.2026.
//

#include "math/UTypes.hpp"

#include <cmath>


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

        TQuat TQuat::fromEulerDegrees(TVec3 pitchYawRoll) {
                return fromEulerRadians({
                        toRadians(pitchYawRoll.x),
                        toRadians(pitchYawRoll.y),
                        toRadians(pitchYawRoll.z)
                });
        }

        TQuat TQuat::fromEulerRadians(TVec3 pitchYawRoll) {
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

        void GTransform::reset() {
                position = TVec3::zero();
                rotation = TQuat::identity();
                scale = TVec3::one();
        }

        void GTransform::setPosition(TVec3 pos) {
                position = pos;
        }

        void GTransform::setRotation(TQuat rot) {
                rotation = rot;
        }

        void GTransform::setScale(TVec3 newScale) {
                scale = newScale;
        }
} // namespace engine::math
