#pragma once

#include "component.hpp"

#include <common/transform.hpp>
#include <utils/uuid.hpp>

#include <vector>

namespace aln
{

/// @brief Entities with a spatial component have a position and orientation in the world.
/// They can be attached to other spatial entities to form hierarchies.
class SpatialComponent : public IComponent
{
  private:
    using UUID = aln::utils::UUID;

    friend class Entity;

    /// @brief List of attached children components.
    std::vector<SpatialComponent*> m_spatialChildren;

    /// @brief Parent component.
    SpatialComponent* m_pSpatialParent = nullptr;
    std::vector<UUID> m_sockets;

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

    bool HasSocket(const aln::utils::UUID& socketID);

    /// @brief Attach this component to another one.
    /// @param pParentComponent: The component to attach to.
    /// @param socketId: TODO
    /// @todo Document Attached/Detached state
    void AttachTo(SpatialComponent* pParentComponent, const aln::utils::UUID& socketID = aln::utils::UUID::InvalidID());

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
    void SetLocalTransformRotation(const glm::quat quat);

    /// @brief Set this transform's rotation in euler angles
    /// @param euler: desired angles in degrees
    void SetLocalTransformRotationEuler(const glm::vec3 euler);

    /// @brief Set this spatial component's position
    void SetLocalTransformPosition(const glm::vec3 pos);

    /// @brief Set this spatial component's scale
    void SetLocalTransformScale(const glm::vec3 scale);

    /// @brief Offset this spatial component's position by a delta
    void OffsetLocalTransformPosition(const glm::vec3 offset);

    /// @brief Offset this spatial component's rotation by a delta
    /// @param quatOffset: delta rotation
    void OffsetLocalTransformRotation(const glm::quat quatOffset);
};
} // namespace aln