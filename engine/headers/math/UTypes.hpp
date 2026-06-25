//
// Created by drhaz on 25.06.2026.
//

#ifndef SMB_UTYPES_HPP
#define SMB_UTYPES_HPP


namespace engine::math {
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
                [[nodiscard]] float length() const;
                [[nodiscard]] TVec3 normalized() const;
                static TVec3 zero() {return {};}
                static TVec3 one() {return {1.0f, 1.0f, 1.0f};}
        };

        TVec3 operator*(float scalar, const TVec3& vec);

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
                static TQuat identity();
                static TQuat fromAxisAngleDegrees(TVec3 axis, float degrees);
                static TQuat fromAxisAngleRadians(TVec3 axis, float radians);
                static TQuat fromEulerDegrees(TVec3 pitchYawRoll);
                static TQuat fromEulerRadians(TVec3 pitchYawRoll);
        };

        struct GTransform {
                TVec3 position = TVec3::zero();
                TQuat rotation = TQuat::identity();
                TVec3 scale = TVec3::one();

                void reset();
                void setPosition(TVec3 pos);
                void setRotation(TQuat rot);
                void setScale(TVec3 newScale);
        };
} // namespace engine::math

#endif //SMB_UTYPES_HPP
