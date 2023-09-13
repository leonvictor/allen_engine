#pragma once

#include "constants.hpp"

#include <aln_common_export.h>

#include <glm/glm.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/vec3.hpp>

namespace aln
{

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

    inline Vec3 Normalized() const { return Vec3(glm::normalize(AsGLM())); }
    inline float Magnitude() const { return glm::length(AsGLM()); }
    inline float SquaredMagnitude() const { return glm::length2(AsGLM()); }

    inline Vec3 ToDegrees() const { return Vec3(glm::degrees(AsGLM())); }
    inline Vec3 ToRadians() const { return Vec3(glm::radians(AsGLM())); }
    Matrix4x4 AsTranslationMatrix() const;
    Matrix4x4 AsScalingMatrix() const;

    inline bool IsNearEqual(const Vec3& other, float eps = Maths::Epsilon) const { return glm::all(glm::epsilonEqual(AsGLM(), other.AsGLM(), eps)); }
    inline bool IsNearZero(float eps = Maths::Epsilon) const { return IsNearEqual(Vec3::Zeroes); }

    Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
    Vec3 operator+(float value) const { return Vec3(x + value, y + value, z + value); }
    Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
    Vec3 operator-(float value) const { return Vec3(x - value, y - value, z - value); }
    Vec3 operator*(const Vec3& other) const { return Vec3(x * other.x, y * other.y, z * other.z); }
    Vec3 operator*(float value) const { return Vec3(x * value, y * value, z * value); }
    Vec3 operator/(const Vec3& other) const { return Vec3(x / other.x, y / other.y, z / other.z); }
    Vec3 operator/(float value) const { return Vec3(x / value, y / value, z / value); }
    bool operator==(const Vec3& other) const { return x == other.x && y == other.y && z == other.z; }

    Vec3 Cross(const Vec3& other) const { return glm::cross(AsGLM(), other.AsGLM()); }
    float Dot(const Vec3& other) const { return glm::dot(AsGLM(), other.AsGLM()); }

    /// @brief Linear interpolation between a and b by a t factor
    inline static Vec3 Lerp(const Vec3& a, const Vec3& b, float t) { return glm::mix(a.AsGLM(), b.AsGLM(), t); }

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

namespace std
{
template <>
struct hash<aln::Vec3>
{
    size_t operator()(const aln::Vec3& vec3) const { return hash<glm::vec3>()(glm::vec3(vec3.x, vec3.y, vec3.z)); }
};
} // namespace std