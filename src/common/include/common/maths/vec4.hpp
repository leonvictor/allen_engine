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

    inline Vec4 Normalized() const { return Vec4(glm::normalize(AsGLM())); }
    inline float Magnitude() const { return glm::length(AsGLM()); }
    inline float SquaredMagnitude() const { return glm::length2(AsGLM()); }

    inline bool IsNearEqual(const Vec4& other, float eps = Maths::Epsilon) const { return glm::all(glm::epsilonEqual(AsGLM(), other.AsGLM(), eps)); }
    inline bool IsNearZero(float eps = Maths::Epsilon) const { return IsNearEqual(Vec4::Zeroes); }

    Vec4 operator+(const Vec4& other) const { return Vec4(x + other.x, y + other.y, z + other.z, w + other.w); }
    Vec4 operator+(float value) const { return Vec4(x + value, y + value, z + value, w + value); }
    Vec4 operator-(const Vec4& other) const { return Vec4(x - other.x, y - other.y, z - other.z, w - other.w); }
    Vec4 operator-(float value) const { return Vec4(x - value, y - value, z - value, w - value); }
    Vec4 operator-() const { return Vec4(-x, -y, -z, -w); }
    Vec4 operator*(const Vec4& other) const { return Vec4(x * other.x, y * other.y, z * other.z, w * other.w); }
    Vec4 operator*(float value) const { return Vec4(x * value, y * value, z * value, w * value); }
    Vec4 operator/(const Vec4& other) const { return Vec4(x / other.x, y / other.y, z / other.z, w / other.w); }
    Vec4 operator/(float value) const { return Vec4(x / value, y / value, z / value, w / value); }
    bool operator==(const Vec4& other) const { return x == other.x && y == other.y && z == other.z && w == other.w; }

    float& operator[](uint8_t idx) {
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
    }

    /// @brief Linear interpolation between a and b by a t factor
    inline static Vec4 Lerp(const Vec4& a, const Vec4& b, float t) { return glm::mix(a.AsGLM(), b.AsGLM(), t); }

    static const Vec4 Zeroes;
    static const Vec4 Ones;
};
} // namespace aln

namespace std
{
template <>
struct hash<aln::Vec4>
{
    size_t operator()(const aln::Vec4& vec4) const { return hash<glm::vec4>()(glm::vec4(vec4.x, vec4.y, vec4.z, vec4.w)); }
};
} // namespace std