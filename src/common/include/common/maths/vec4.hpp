#pragma once

#include "constants.hpp"
#include <aln_common_export.h>

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/vec4.hpp>

namespace aln
{

class Vec3;

class ALN_COMMON_EXPORT Vec4
{
    friend class Vec3;
    friend class Matrix4x4;
    friend class Quaternion;

  private:
    Vec4(const glm::vec4& vec4) : x(vec4.x), y(vec4.y), z(vec4.z), w(vec4.w) {}
    glm::vec4 AsGLM() const { return glm::vec4(x, y, z, w); }

  public:
    float x, y, z, w;

    Vec4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
    Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
    Vec4(const Vec3& xyz, float w);

    Vec4 operator-() const { return Vec4(-x, -y, -z, -w); }
    
    Vec4 operator+(const Vec4& other) const { return Vec4(x + other.x, y + other.y, z + other.z, w + other.w); }
    Vec4 operator-(const Vec4& other) const { return Vec4(x - other.x, y - other.y, z - other.z, w - other.w); }
    /// @brief Component-wise product
    Vec4 operator*(const Vec4& other) const { return Vec4(x * other.x, y * other.y, z * other.z, w * other.w); }
    /// @brief Component-wise division
    Vec4 operator/(const Vec4& other) const { return Vec4(x / other.x, y / other.y, z / other.z, w * other.w); }

    Vec4 operator+(float value) const { return Vec4(x + value, y + value, z + value, w + value); }
    Vec4 operator-(float value) const { return Vec4(x - value, y - value, z - value, w - value); }
    Vec4 operator*(float value) const { return Vec4(x * value, y * value, z * value, w * value); }
    Vec4 operator/(float value) const { return Vec4(x / value, y / value, z / value, w / value); }

    friend Vec4 operator+(float value, const Vec4& vec) { return vec + value; }
    friend Vec4 operator-(float value, const Vec4& vec) { return Vec4(value - vec.x, value - vec.y, value - vec.z, value - vec.w); }
    friend Vec4 operator*(float value, const Vec4& vec) { return vec * value; }
    friend Vec4 operator/(float value, const Vec4& vec) { return Vec4(value / vec.x, value / vec.y, value / vec.z, value / vec.w); }

    Vec4& operator+=(const Vec4& other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        w += other.w;
        return *this;
    }
    Vec4& operator-=(const Vec4& other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        w -= other.w;
        return *this;
    }
    /// @brief Component-wise product
    Vec4& operator*=(const Vec4& other)
    {
        x *= other.x;
        y *= other.y;
        z *= other.z;
        w *= other.w;
        return *this;
    }
    /// @brief Component-wise division
    Vec4& operator/=(const Vec4& other)
    {
        x /= other.x;
        y /= other.y;
        z /= other.z;
        w /= other.w;
        return *this;
    }
    
    Vec4& operator+=(float value)
    {
        x += value;
        y += value;
        z += value;
        w += value;
        return *this;
    }
    Vec4& operator-=(float value)
    {
        x -= value;
        y -= value;
        z -= value;
        w -= value;
        return *this;
    }
    Vec4& operator*=(float value)
    {
        x *= value;
        y *= value;
        z *= value;
        w *= value;
        return *this;
    }
    Vec4& operator/=(float value)
    {
        x /= value;
        y /= value;
        z /= value;
        w /= value;
        return *this;
    }
   
    inline bool IsNearEqual(const Vec4& other, float eps = Maths::Epsilon) const { return glm::all(glm::epsilonEqual(AsGLM(), other.AsGLM(), eps)); }
    inline bool IsNearZero(float eps = Maths::Epsilon) const { return IsNearEqual(Vec4::Zeroes); }
    bool operator==(const Vec4& other) const { return IsNearEqual(other); }
    bool operator!=(const Vec4& other) const { return !IsNearEqual(other); }

    float& operator[](uint8_t idx)
    {
        assert(idx < 4);
        switch (idx)
        {
        case 0:
            return x;
        case 1:
            return y;
        case 2:
            return z;
        case 3:
            return w;
        }
        assert(false);
        return x;
    }

    inline float Magnitude() const { return glm::length(AsGLM()); }
    inline float SquaredMagnitude() const { return glm::length2(AsGLM()); }
    inline Vec4 Sign() const { return glm::sign(AsGLM()); }

    inline Vec4 Normalized() const { return Vec4(glm::normalize(AsGLM())); }

    /// @brief Linear interpolation between a and b by a t factor
    inline static Vec4 Lerp(const Vec4& a, const Vec4& b, float t) { return glm::mix(a.AsGLM(), b.AsGLM(), t); }

    static const Vec4 Zeroes;
    static const Vec4 Ones;
};
} // namespace aln
