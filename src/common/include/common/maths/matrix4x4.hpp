#pragma once

#include <aln_common_export.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include <array>

namespace aln
{
class Transform;

class ALN_COMMON_EXPORT Matrix4x4
{
    friend class Vec3;
    friend class Quaternion;

    using ColumnType = std::array<float, 4>;

  private:
    Matrix4x4(const glm::mat4x4& matrix)
        : columns{{{matrix[0][0], matrix[0][1], matrix[0][2], matrix[0][3]},
              {matrix[1][0], matrix[1][1], matrix[1][2], matrix[1][3]},
              {matrix[2][0], matrix[2][1], matrix[2][2], matrix[2][3]},
              {matrix[3][0], matrix[3][1], matrix[3][2], matrix[3][3]}}} {}

    glm::mat4x4 AsGLM() const
    {
        return glm::mat4x4(
            columns[0][0], columns[0][1], columns[0][2], columns[0][3],
            columns[1][0], columns[1][1], columns[1][2], columns[1][3],
            columns[2][0], columns[2][1], columns[2][2], columns[2][3],
            columns[3][0], columns[3][1], columns[3][2], columns[3][3]);
    }

  public:
    std::array<ColumnType, 4> columns;

    Matrix4x4() = default;

    ColumnType& operator[](uint8_t idx)
    {
        assert(idx < 4);
        return columns[idx];
    }

    Transform AsTransform() const;

    Matrix4x4 operator*(const Matrix4x4& other) { return Matrix4x4(AsGLM() * other.AsGLM()); }

    /// @brief Construct a perspective matrix
    /// @param fov Field of view (in degrees)
    static Matrix4x4 Perspective(float fov, float aspectRatio, float nearPlane, float farPlane)
    {
        return Matrix4x4(glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane));
    }

    static Matrix4x4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up);
};

} // namespace aln