#include "maths/matrix4x4.hpp"

#include "maths/quaternion.hpp"
#include "maths/vec3.hpp"
#include "transform.hpp"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace aln
{

Transform Matrix4x4::AsTransform() const
{
    glm::vec3 skew;
    glm::vec4 perspective;
    glm::vec3 scale;
    glm::vec3 translation;
    glm::quat rotation;

    glm::decompose(AsGLM(), scale, rotation, translation, skew, perspective);

    return Transform(Vec3(translation), Quaternion(rotation).Normalized(), Vec3(scale));
}

Matrix4x4 Matrix4x4::LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) { return Matrix4x4(glm::lookAt(eye.AsGLM(), center.AsGLM(), up.AsGLM())); }

} // namespace aln