#pragma once

#include "component.hpp"

#include <common/containers/vector.hpp>
#include <common/maths/angles.hpp>
#include <common/transform.hpp>
#include <common/uuid.hpp>
#include <reflection/type_info.hpp>

namespace aln
{

/// @brief Entities with a spatial component have a position and orientation in the world.
/// They can be attached to other spatial entities to form hierarchies.
class SpatialComponent : public IComponent
{
    ALN_REGISTER_TYPE()

  private:
    friend class Entity;
    friend class EntityDescriptor;
    friend class EntityInspector;

    /// @brief List of attached children components.
    Vector<SpatialComponent*> m_spatialChildren;

    /// @brief Parent component.
    SpatialComponent* m_pSpatialParent = nullptr;
    Vector<UUID> m_sockets;

    // TODO: Handle socket
    UUID m_parentAttachmentSocketID;

    Transform m_localTransform;
    Transform m_worldTransform;

    // TODO: Local/world bounds (oriented bounding boxes)

    /// @brief Calculate the world transform according to the parent's component world transform and our own local one.
    /// @param callback: whether to trigger the callback to calculate the component's children's world transform.
    void CalculateWorldTransform(bool callback = true);

  public:
    virtual ~SpatialComponent() {}

    bool HasSocket(const UUID& socketID);

    inline bool HasChildren() const { return m_spatialChildren.empty(); }

    /// @brief Attach this component to another one.
    /// @param pParentComponent: The component to attach to.
    /// @param socketId: TODO
    /// @todo Document Attached/Detached state
    void AttachTo(SpatialComponent* pParentComponent, const aln::UUID& socketID = UUID::InvalidID);

    /// @brief Detach this component from its parent.
    void Detach();

    /// @brief Get the world transform of this component.
    const Transform& GetWorldTransform() const { return m_worldTransform; }

    /// @brief Get the local transform of this component.
    const Transform& GetLocalTransform() const { return m_localTransform; }

    /// @brief Set the local transform of this component. Will also update the world positions of all children.
    void SetLocalTransform(const Transform& transform)
    {
        m_localTransform = transform;
        CalculateWorldTransform(true);
    }

    /// @brief Set this transform's rotation in quaternions
    void SetLocalTransformRotation(const Quaternion& quat);

    /// @brief Set this transform's rotation in euler angles
    /// @param euler: desired angles in degrees
    void SetLocalTransformRotationEuler(const EulerAnglesDegrees& euler);

    /// @brief Set this spatial component's position
    /// @todo Rename to "translation" to match transforms
    void SetLocalTransformPosition(const Vec3& pos);

    /// @brief Set this spatial component's scale
    void SetLocalTransformScale(const Vec3& scale);

    void SetLocalTransformPositionAndRotation(const Vec3& pos, const Quaternion& rotation);

    /// @brief Offset this spatial component's position by a delta
    void OffsetLocalTransformPosition(const Vec3& offset);

    /// @brief Offset this spatial component's rotation by a delta
    /// @param quatOffset: delta rotation
    void OffsetLocalTransformRotation(const Quaternion& quatOffset);
};
} // namespace aln