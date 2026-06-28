//
// Created by drhaz on 25.06.2026.
//

#ifndef SMB_UTYPES_HPP
#define SMB_UTYPES_HPP

#include <array>
#include <cstddef>


namespace engine::math {
        struct Transform;

        struct TVec3 {
                float x{0.0f};
                float y{0.0f};
                float z{0.0f};

                TVec3() = default;
                TVec3(float x1, float y1, float z1) : x(x1), y(y1), z(z1) {}
                TVec3(const TVec3&) = default;
                TVec3(TVec3&&) noexcept = default;

                TVec3& operator+=(const TVec3& other);
                TVec3& operator-=(const TVec3& other);
                TVec3& operator*=(const TVec3& other);
                TVec3& operator/=(const TVec3& other);
                TVec3& operator*=(float scalar);
                TVec3& operator/=(float scalar);
                TVec3 operator+(const TVec3& other) const;
                TVec3 operator-(const TVec3& other) const;
                TVec3 operator*(const TVec3& other) const;
                TVec3 operator/(const TVec3& other) const;
                TVec3 operator*(float scalar) const;
                TVec3 operator/(float scalar) const;
                TVec3 operator-() const;
                TVec3& operator=(const TVec3& other);
                TVec3& operator=(TVec3&& other) noexcept;
                bool operator==(const TVec3& other) const;
                bool operator!=(const TVec3& other) const;

                [[nodiscard]] float dot(const TVec3& other) const;
                [[nodiscard]] TVec3 cross(const TVec3& other) const;
                [[nodiscard]] float length() const;
                [[nodiscard]] TVec3 normalized() const;
                static TVec3 zero() {return {};}
                static TVec3 one() {return {1.0f, 1.0f, 1.0f};}
        };

        struct TVec2 {
                float x{0.0f};
                float y{0.0f};

                TVec2() = default;
                TVec2(float x1, float y1) : x(x1), y(y1) {}
                TVec2(const TVec2&) = default;
                TVec2(TVec2&&) noexcept = default;

                TVec2& operator+=(const TVec2& other);
                TVec2& operator-=(const TVec2& other);
                TVec2& operator*=(const TVec2& other);
                TVec2& operator/=(const TVec2& other);
                TVec2& operator*=(float scalar);
                TVec2& operator/=(float scalar);
                TVec2 operator+(const TVec2& other) const;
                TVec2 operator-(const TVec2& other) const;
                TVec2 operator*(const TVec2& other) const;
                TVec2 operator/(const TVec2& other) const;
                TVec2 operator*(float scalar) const;
                TVec2 operator/(float scalar) const;
                TVec2 operator-() const;
                TVec2& operator=(const TVec2& other);
                TVec2& operator=(TVec2&& other) noexcept;
                bool operator==(const TVec2& other) const;
                bool operator!=(const TVec2& other) const;

                [[nodiscard]] float dot(const TVec2& other) const;
                [[nodiscard]] float length() const;
                [[nodiscard]] TVec2 normalized() const;
                static TVec2 zero() {return {};}
                static TVec2 one() {return {1.0f, 1.0f};}
        };

        TVec3 operator*(float scalar, const TVec3& vec);
        TVec2 operator*(float scalar, const TVec2& vec);

        struct TQuat {
                float w{1.0f};
                float x{0.0f};
                float y{0.0f};
                float z{0.0f};

                TQuat() = default;
                TQuat(float w1, float x1, float y1, float z1) : w(w1), x(x1), y(y1), z(z1) {}
                TQuat(const TQuat&) = default;
                TQuat(TQuat&&) noexcept = default;

                TQuat& operator*=(const TQuat& other);
                TQuat operator*(const TQuat& other) const;
                TQuat& operator=(const TQuat& other);
                TQuat& operator=(TQuat&& other) noexcept;
                bool operator==(const TQuat& other) const;
                bool operator!=(const TQuat& other) const;
                [[nodiscard]] float length() const;
                [[nodiscard]] TQuat normalized() const;
                [[nodiscard]] TVec3 rotate(TVec3 vector) const;
                static TQuat identity();
                static TQuat fromAxisAngleDegrees(TVec3 axis, float degrees);
                static TQuat fromAxisAngleRadians(TVec3 axis, float radians);
                static TQuat fromEulerXYZDegrees(TVec3 pitchYawRoll);
                static TQuat fromEulerXYZRadians(TVec3 pitchYawRoll);
        };

        struct TMat4 {
                std::array<float, 16> m{};

                TMat4();
                explicit TMat4(float diagonal);
                explicit TMat4(std::array<float, 16> values);

                [[nodiscard]] float* data();
                [[nodiscard]] const float* data() const;
                [[nodiscard]] float& operator[](std::size_t index);
                [[nodiscard]] const float& operator[](std::size_t index) const;

                TMat4& operator*=(const TMat4& other);
                [[nodiscard]] TMat4 operator*(const TMat4& other) const;
                bool operator==(const TMat4& other) const;
                bool operator!=(const TMat4& other) const;

                static TMat4 zero();
                static TMat4 identity();
                static TMat4 translation(TVec3 translation);
                static TMat4 rotation(TQuat rotation);
                static TMat4 scale(TVec3 scale);
                static TMat4 fromTransform(const Transform& transform);
                static TMat4 lookAtLeftHanded(TVec3 eye, TVec3 at, TVec3 up);

        };

        struct Transform {
                TVec3 position = TVec3::zero();
                TQuat rotation = TQuat::identity();
                TVec3 scale = TVec3::one();

                void reset();
                void setPosition(TVec3 pos);
                void setRotation(TQuat rot);
                void setScale(TVec3 newScale);
        };

        struct TColor {
                float r{0.0f};
                float g{0.0f};
                float b{0.0f};
                float a{1.0f};

                
        };
} // namespace engine::math

#endif //SMB_UTYPES_HPP
