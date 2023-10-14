#pragma once

#include <common/colors.hpp>
#include <common/maths/angles.hpp>
#include <common/maths/matrix4x4.hpp>
#include <common/maths/vec2.hpp>
#include <common/maths/vec3.hpp>
#include <entities/spatial_component.hpp>
#include <entities/update_context.hpp>
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
    float m_fieldOfView = 45.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 300.f;

    // TODO: Optionnaly render the skybox as background
    RGBAColor m_backgroundColor = {0, 0, 0, 255};

    Matrix4x4 GetViewMatrix() const;
    // TODO: aspectRatio could be kept in a similar class representing a camera view
    Matrix4x4 GetProjectionMatrix(float aspectRatio) const;
    Matrix4x4 GetViewProjectionMatrix(float aspectRatio) const;

    Vec3 GetCameraForwardVector() const { return GetWorldTransform().GetForwardVector(); }
    Vec3 GetCameraUpVector() const { return GetWorldTransform().GetUpVector(); }
    Vec3 GetCameraRightVector() const { return GetWorldTransform().GetRightVector(); }

    void Initialize() override {}
    void Shutdown() override {}
    void Load(const LoadingContext& loadingContext) override {}
    void Unload(const LoadingContext& loadingContext) override {}
};

/// @brief A camera that orbits around its parent component's transform
/// @note Inspired from https://catlikecoding.com/unity/tutorials/movement/orbit-camera/
class OrbitCameraComponent : public CameraComponent
{
    ALN_REGISTER_TYPE()

  private:
    Transform m_currentOrbitTransform = Transform::Identity; // Transform from the orbit target center to the camera

    // TODO: yaw, pitch in radians
    Degrees m_pitch = 45.0f;
    Degrees m_yaw = 0.0f;

    Vec3 m_currentFocusPosition; // Where the camera is pointing to (can differ from actual parent's position due to easing and offsets)

    // Angle constraints
    Degrees m_pitchAngleMin = 0;
    Degrees m_pitchAngleMax = 60.0f;

    // Offsets
    Vec3 m_focusOffset = Vec3::Zeroes;
    float m_orbitDistance = 5.0f;  // Distance from the focus point
     
    // Deadzone
    float m_deadzoneRadius = 1.0f;    // Do not move the camera if the focus is in the radius
    float m_deadzoneCentering = 0.5f; // [0, 1]

private:
    void ConstrainAngles()
    {
        m_pitch = Degrees::Clamp(m_pitch, m_pitchAngleMin, m_pitchAngleMax);
        m_yaw = Degrees::Wrap360(m_yaw);
    }

    void UpdateFocusPoint(float deltaTime)
    {
        // Check whether the focus point is out of the focus radius
        auto desiredFocusPosition = GetSpatialParent()->GetWorldTransform().GetTranslation() + m_focusOffset;

        if (m_deadzoneRadius > 0.0f)
        {
            auto distance = Vec3::Distance(desiredFocusPosition, m_currentFocusPosition);

            // Smoothly center the camera on the focus rather than staying on the radius' edge
            float t = 1.0f;
            if (distance > 0.01f && m_deadzoneCentering > 0.0f)
            {
                t = Maths::Pow(1.0f - m_deadzoneCentering, deltaTime);
            }
            if (distance > m_deadzoneRadius)
            {
                t = Maths::Min(t, m_deadzoneRadius / distance);
            }
            m_currentFocusPosition = Vec3::Lerp(desiredFocusPosition, m_currentFocusPosition, t);
        }
        else
        {
            m_currentFocusPosition = desiredFocusPosition;
        }

        m_currentFocusPosition = desiredFocusPosition;
    }
  public:
    void SetFocusOffset(const Vec3& offset) { m_focusOffset = offset; }

    void OffsetOrbitAngle(const Degrees& deltaYawAngle, const Degrees& deltaPitchAngle)
    {
        m_yaw += deltaYawAngle;
        m_pitch += deltaPitchAngle;

        ConstrainAngles();
    }

    void OffsetOrbitDistance(float deltaDistance) { m_orbitDistance += deltaDistance; }
    void OffsetOrbitFocusOffset(const Vec3& deltaFocusOffset) { m_focusOffset += deltaFocusOffset; }

    void RotateHorizontallyTowards(const Degrees& desiredYaw, const Degrees& maxDelta)
    {
        m_yaw = Degrees::StepTowards(m_yaw, desiredYaw, maxDelta);
        // TODO: Handle smooth alignment
        ConstrainAngles();
    }

    /// @brief Call after adjustments to set the camera transform
    void FinalizeUpdate(float deltaTime)
    {
        UpdateFocusPoint(deltaTime);

        // Update camera world transform
        auto lookRotation = Quaternion::FromEulerAngles(EulerAnglesDegrees(m_yaw, m_pitch, 0.0f).ToRadians());
        auto lookDirection = lookRotation.RotateVector(Vec3::WorldForward);
        auto lookPosition = m_currentFocusPosition - lookDirection * m_orbitDistance;

        m_currentOrbitTransform.SetTranslation(lookPosition);
        m_currentOrbitTransform.SetRotation(lookRotation);

        SetWorldTransform(m_currentOrbitTransform);
    }


};
} // namespace aln