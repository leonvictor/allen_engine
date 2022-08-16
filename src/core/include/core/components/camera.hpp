#pragma once

#include <common/colors.hpp>
#include <entities/spatial_component.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace aln
{
class Camera : public entities::SpatialComponent
{
    ALN_REGISTER_TYPE();

    friend class EditorCameraController;

    enum class Type
    {
        Game,      // In-game camera (todo)
        SceneView, // Renders the scene view in the editor
        Preview,   // Renders to previews (todo)
    };

    Type m_type = Type::SceneView;

  public:
    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 right;

    // TODO: This shouldn't be in the camera component.
    glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);

    // TODO
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 300.f;

    // TODO: Optionnaly render the skybox as background
    RGBAColor m_backgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};

    glm::mat4 GetViewMatrix() const;
    // TODO: aspectRatio could be kept in a similar class representing a camera view
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;
    glm::mat4 GetViewProjectionMatrix(float aspectRatio) const;

    void Initialize() override {}
    void Shutdown() override {}
    void Load(const entities::LoadingContext& loadingContext) override {}
    void Unload(const entities::LoadingContext& loadingContext) override {}
};
} // namespace aln