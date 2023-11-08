#pragma once

#include "constants.hpp"
#include "trig.hpp"

#include <aln_common_export.h>

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/vec3.hpp>

namespace aln
{

class Matrix4x4;

class ALN_COMMON_EXPORT Vec3
{
    friend class Matrix4x4;
    friend class Quaternion;

  private:
    Vec3(const glm::vec3& vec3) : x(vec3.x), y(vec3.y), z(vec3.z) {}
    glm::vec3 AsGLM() const { return glm::vec3(x, y, z); }

  public:
    float x, y, z;

    Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
    Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

    Matrix4x4 ToTranslationMatrix() const;
    Matrix4x4 ToScalingMatrix() const;

    Vec3 operator-() const { return Vec3(-x, -y, -z); }

    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    /// @brief Component-wise product
    Vec3 operator*(const Vec3& other) const { return Vec3(x * other.x, y * other.y, z * other.z); }
    /// @brief Component-wise division
    Vec3 operator/(const Vec3& other) const { return Vec3(x / other.x, y / other.y, z / other.z); }

    Vec3 operator+(float value) const { return Vec3(x + value, y + value, z + value); }
    Vec3 operator-(float value) const { return Vec3(x - value, y - value, z - value); }
    Vec3 operator*(float value) const { return Vec3(x * value, y * value, z * value); }
    Vec3 operator/(float value) const { return Vec3(x / value, y / value, z / value); }

    friend Vec3 operator+(float value, const Vec3& vec) { return vec + value; }
    friend Vec3 operator-(float value, const Vec3& vec) { return Vec3(value - vec.x, value - vec.y, value - vec.z); }
    friend Vec3 operator*(float value, const Vec3& vec) { return vec * value; }
    friend Vec3 operator/(float value, const Vec3& vec) { return Vec3(value / vec.x, value / vec.y, value / vec.z); }

    Vec3& operator+=(const Vec3& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    Vec3& operator-=(const Vec3& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    /// @brief Component-wise product
    Vec3& operator*=(const Vec3& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        return *this;
    }
    /// @brief Component-wise division
    Vec3& operator/=(const Vec3& other)
    {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        return *this;
    }

    Vec3& operator+=(float value)
    {
        x += value;
        y += value;
        z += value;
        return *this;
    }
    Vec3& operator-=(float value)
    {
        x -= value;
        y -= value;
        z -= value;
        return *this;
    }
    Vec3& operator*=(float value)
    {
        x *= value;
        y *= value;
        z *= value;
        return *this;
    }
    Vec3& operator/=(float value)
    {
        x /= value;
        y /= value;
        z /= value;
        return *this;
    }

    inline bool IsNearEqual(const Vec3& other, float eps = Maths::Epsilon) const { return glm::all(glm::epsilonEqual(AsGLM(), other.AsGLM(), eps)); }
    inline bool IsNearZero(float eps = Maths::Epsilon) const { return IsNearEqual(Vec3::Zeroes); }
    bool operator==(const Vec3& other) const { return IsNearEqual(other); }
    bool operator!=(const Vec3& other) const { return !IsNearEqual(other); }

    float& operator[](uint8_t idx)
    {
        assert(idx < 3);
        switch (idx)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        }
        assert(false);
        return x;
    }

    Vec3 Scale(const Vec3& other) const { return *this * other; }
    Vec3 Translate(const Vec3& other) const { return *this + other; }
    Vec3 Cross(const Vec3& other) const { return glm::cross(AsGLM(), other.AsGLM()); }
    float Dot(const Vec3& other) const { return glm::dot(AsGLM(), other.AsGLM()); }

    inline bool IsNormalized() const {return Maths::IsNearEqual(Magnitude(), 1.0); }
    inline float Magnitude() const { return glm::length(AsGLM()); }
    inline float SquaredMagnitude() const { return glm::length2(AsGLM()); }
    inline Vec3 Sign() const { return glm::sign(AsGLM()); }

    inline Vec3 Normalized() const { return Vec3(glm::normalize(AsGLM())); }

    /// @brief Linear interpolation between a and b by a t factor
    inline static Vec3 Lerp(const Vec3& a, const Vec3& b, float t) { return glm::mix(a.AsGLM(), b.AsGLM(), t); }
    inline static float Distance(const Vec3& a, const Vec3& b) { return glm::distance(a.AsGLM(), b.AsGLM()); }
    /// @brief Smallest angle between two vectors
    inline static Radians Angle(const Vec3& from, const Vec3& to)
    {
        auto denominator = (float) Maths::Sqrt(from.SquaredMagnitude() * to.SquaredMagnitude());
        if (denominator < Maths::Epsilon)
        {
            return 0.0f;
        }

        float dot = Maths::Clamp(from.Dot(to) / denominator, -1.0f, 1.0f);
        return Maths::Acos(dot);
    }

    static const Vec3 Zeroes;
    static const Vec3 Ones;
    static const Vec3 X;
    static const Vec3 Y;
    static const Vec3 Z;
    static const Vec3 WorldUp;
    static const Vec3 WorldDown;
    static const Vec3 WorldRight;
    static const Vec3 WorldLeft;
    static const Vec3 WorldForward;
    static const Vec3 WorldBackward;
};
} // namespace aln
