#pragma once

#include "constants.hpp"

#include <aln_common_export.h>

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/vec2.hpp>

namespace aln
{

class ALN_COMMON_EXPORT Vec2
{
    friend class Vec3;
    friend class Matrix4x4;
    friend class Quaternion;

  private:
    Vec2(const glm::vec2& vec) : x(vec.x), y(vec.y) {}
    glm::vec2 AsGLM() const { return glm::vec2(x, y); }

  public:
    float x, y;

    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float x, float y) : x(x), y(y) {}

    Vec2 operator-() const { return Vec2(-x, -y); }

    Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
    Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
    /// @brief Component-wise product
    Vec2 operator*(const Vec2& other) const { return Vec2(x * other.x, y * other.y); }
    /// @brief Component-wise division
    Vec2 operator/(const Vec2& other) const { return Vec2(x / other.x, y / other.y); }

    Vec2 operator+(float value) const { return Vec2(x + value, y + value); }
    Vec2 operator-(float value) const { return Vec2(x - value, y - value); }
    Vec2 operator*(float value) const { return Vec2(x * value, y * value); }
    Vec2 operator/(float value) const { return Vec2(x / value, y / value); }

    friend Vec2 operator+(float value, const Vec2& vec) { return vec + value; }
    friend Vec2 operator-(float value, const Vec2& vec) { return Vec2(value - vec.x, value - vec.y); }
    friend Vec2 operator*(float value, const Vec2& vec) { return vec * value; };
    friend Vec2 operator/(float value, const Vec2& vec) { return Vec2(value / vec.x, value / vec.y); }

    Vec2& operator+=(const Vec2& other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }
    Vec2& operator-=(const Vec2& other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    /// @brief Component-wise product
    Vec2& operator*=(const Vec2& other)
    {
        x *= other.x;
        y *= other.y;
        return *this;
    }
    /// @brief Component-wise division
    Vec2& operator/=(const Vec2& other)
    {
        x /= other.x;
        y /= other.y;
        return *this;
    }

    Vec2& operator+=(float value)
    {
        x += value;
        y += value;
        return *this;
    }
    Vec2& operator-=(float value)
    {
        x -= value;
        y -= value;
        return *this;
    }
    Vec2& operator*=(float value)
    {
        x *= value;
        y *= value;
        return *this;
    }
    Vec2& operator/=(float value)
    {
        x /= value;
        y /= value;
        return *this;
    }

    inline bool IsNearEqual(const Vec2& other, float eps = Maths::Epsilon) const { return glm::all(glm::epsilonEqual(AsGLM(), other.AsGLM(), eps)); }
    inline bool IsNearZero(float eps = Maths::Epsilon) const { return IsNearEqual(Vec2::Zeroes, eps); }
    bool operator==(const Vec2& other) const { return IsNearEqual(other); }
    bool operator!=(const Vec2& other) const { return !IsNearEqual(other); }

    float& operator[](uint8_t idx)
    {
        assert(idx < 2);
        switch (idx)
        {
        case 0:
            return x;
        case 1:
            return y;
        }
        assert(false);
        return x;
    }

    inline float Magnitude() const { return glm::length(AsGLM()); }
    inline float SquaredMagnitude() const { return glm::length2(AsGLM()); }
    inline Vec2 Sign() const { return glm::sign(AsGLM()); }

    inline Vec2 Normalized() const { return Vec2(glm::normalize(AsGLM())); }

    Vec2 Scale(const Vec2& other) const { return *this * other; }
    Vec2 Translate(const Vec2& other) const { return *this + other; }

    /// @brief Linear interpolation between a and b by a t factor
    inline static Vec2 Lerp(const Vec2& a, const Vec2& b, float t) { return glm::mix(a.AsGLM(), b.AsGLM(), t); }

    static const Vec2 Zeroes;
    static const Vec2 Ones;
};
} // namespace aln
