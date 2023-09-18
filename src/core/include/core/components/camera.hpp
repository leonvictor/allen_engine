#pragma once

#include <common/colors.hpp>
#include <common/maths/matrix4x4.hpp>
#include <common/maths/vec3.hpp>
#include <entities/spatial_component.hpp>
#include <reflection/type_info.hpp>

namespace aln
{
class CameraComponent : public SpatialComponent
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
    Vec3 forward;
    Vec3 up;
    Vec3 right;

    // TODO: This shouldn't be in the camera component.
    Vec3 world_up = Vec3(0.0f, 1.0f, 0.0f);

    // TODO
    float m_fieldOfView = 45.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 300.f;

    // TODO: Optionnaly render the skybox as background
    RGBAColor m_backgroundColor = {0, 0, 0, 255};

    Matrix4x4 GetViewMatrix() const;
    // TODO: aspectRatio could be kept in a similar class representing a camera view
    Matrix4x4 GetProjectionMatrix(float aspectRatio) const;
    Matrix4x4 GetViewProjectionMatrix(float aspectRatio) const;

    void Initialize() override {}
    void Shutdown() override {}
    void Load(const LoadingContext& loadingContext) override {}
    void Unload(const LoadingContext& loadingContext) override {}
};
} // namespace aln