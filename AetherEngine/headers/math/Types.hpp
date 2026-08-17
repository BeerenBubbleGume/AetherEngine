//
// Created by drhaz on 25.06.2026.
//

#ifndef SMB_MATH_TYPES_HPP
#define SMB_MATH_TYPES_HPP

#include <array>
#include <cstddef>


namespace AetherEngine::math {
        struct Transform;

        struct Vec3 {
                float x{0.0f};
                float y{0.0f};
                float z{0.0f};

                Vec3() = default;
                Vec3(float x1, float y1, float z1) : x(x1), y(y1), z(z1) {}
                Vec3(const Vec3&) = default;
                Vec3(Vec3&&) noexcept = default;

                Vec3& operator+=(const Vec3& other);
                Vec3& operator-=(const Vec3& other);
                Vec3& operator*=(const Vec3& other);
                Vec3& operator/=(const Vec3& other);
                Vec3& operator*=(float scalar);
                Vec3& operator/=(float scalar);
                Vec3 operator+(const Vec3& other) const;
                Vec3 operator-(const Vec3& other) const;
                Vec3 operator*(const Vec3& other) const;
                Vec3 operator/(const Vec3& other) const;
                Vec3 operator*(float scalar) const;
                Vec3 operator/(float scalar) const;
                Vec3 operator-() const;
                Vec3& operator=(const Vec3& other);
                Vec3& operator=(Vec3&& other) noexcept;
                bool operator==(const Vec3& other) const;
                bool operator!=(const Vec3& other) const;

                [[nodiscard]] float dot(const Vec3& other) const;
                [[nodiscard]] Vec3 cross(const Vec3& other) const;
                [[nodiscard]] float length() const;
                [[nodiscard]] Vec3 normalized() const;
                static Vec3 zero() {return {};}
                static Vec3 one() {return {1.0f, 1.0f, 1.0f};}
        };

        struct Vec2 {
                float x{0.0f};
                float y{0.0f};

                Vec2() = default;
                Vec2(float x1, float y1) : x(x1), y(y1) {}
                Vec2(const Vec2&) = default;
                Vec2(Vec2&&) noexcept = default;

                Vec2& operator+=(const Vec2& other);
                Vec2& operator-=(const Vec2& other);
                Vec2& operator*=(const Vec2& other);
                Vec2& operator/=(const Vec2& other);
                Vec2& operator*=(float scalar);
                Vec2& operator/=(float scalar);
                Vec2 operator+(const Vec2& other) const;
                Vec2 operator-(const Vec2& other) const;
                Vec2 operator*(const Vec2& other) const;
                Vec2 operator/(const Vec2& other) const;
                Vec2 operator*(float scalar) const;
                Vec2 operator/(float scalar) const;
                Vec2 operator-() const;
                Vec2& operator=(const Vec2& other);
                Vec2& operator=(Vec2&& other) noexcept;
                bool operator==(const Vec2& other) const;
                bool operator!=(const Vec2& other) const;

                [[nodiscard]] float dot(const Vec2& other) const;
                [[nodiscard]] float length() const;
                [[nodiscard]] Vec2 normalized() const;
                static Vec2 zero() {return {};}
                static Vec2 one() {return {1.0f, 1.0f};}
        };

        Vec3 operator*(float scalar, const Vec3& vec);
        Vec2 operator*(float scalar, const Vec2& vec);

        struct Quat {
                float w{1.0f};
                float x{0.0f};
                float y{0.0f};
                float z{0.0f};

                Quat() = default;
                Quat(float w1, float x1, float y1, float z1) : w(w1), x(x1), y(y1), z(z1) {}
                Quat(const Quat&) = default;
                Quat(Quat&&) noexcept = default;

                Quat& operator*=(const Quat& other);
                Quat operator*(const Quat& other) const;
                Quat& operator=(const Quat& other);
                Quat& operator=(Quat&& other) noexcept;
                bool operator==(const Quat& other) const;
                bool operator!=(const Quat& other) const;
                [[nodiscard]] float length() const;
                [[nodiscard]] Quat normalized() const;
                [[nodiscard]] Vec3 rotate(Vec3 vector) const;
                static Quat identity();
                static Quat fromAxisAngleDegrees(Vec3 axis, float degrees);
                static Quat fromAxisAngleRadians(Vec3 axis, float radians);
                static Quat fromEulerXYZDegrees(Vec3 pitchYawRoll);
                static Quat fromEulerXYZRadians(Vec3 pitchYawRoll);
        };

        struct Mat4 {
                std::array<float, 16> m{};

                Mat4();
                explicit Mat4(float diagonal);
                explicit Mat4(std::array<float, 16> values);

                [[nodiscard]] float* data();
                [[nodiscard]] const float* data() const;
                [[nodiscard]] float& operator[](std::size_t index);
                [[nodiscard]] const float& operator[](std::size_t index) const;

                Mat4& operator*=(const Mat4& other);
                [[nodiscard]] Mat4 operator*(const Mat4& other) const;
                bool operator==(const Mat4& other) const;
                bool operator!=(const Mat4& other) const;

                static Mat4 zero();
                static Mat4 identity();
                static Mat4 translation(Vec3 translation);
                static Mat4 rotation(Quat rotation);
                static Mat4 scale(Vec3 scale);
                static Mat4 fromTransform(const Transform& transform);
                static Mat4 lookAtLeftHanded(Vec3 eye, Vec3 at, Vec3 up);

        };

        struct Transform {
                Vec3 position = Vec3::zero();
                Quat rotation = Quat::identity();
                Vec3 scale = Vec3::one();

                void reset();
                void setPosition(Vec3 pos);
                void setRotation(Quat rot);
                void setScale(Vec3 newScale);
        };

        struct Color {
                float r{0.0f};
                float g{0.0f};
                float b{0.0f};
                float a{1.0f};

                
        };
} // namespace AetherEngine::math

#endif //SMB_MATH_TYPES_HPP
