#pragma once

#include "../transform2.hpp"
#include "../utils/uuid.hpp"
#include "component.hpp"

#include <vector>

/// @brief Entities with a spatial component have a position and orientation in the world.
/// They can be attached to other spatial entities to form hierarchies.
class SpatialComponent : public IComponent
{
  private:
    friend class Entity;

    /// @brief List of attached children components.
    std::vector<SpatialComponent*> m_spatialChildren;

    /// @brief Parent component.
    SpatialComponent* m_pSpatialParent = nullptr;
    std::vector<core::UUID> m_sockets;

    // TODO: Handle socket
    core::UUID m_parentAttachmentSocketID;

    Transform m_localTransform;
    Transform m_worldTransform;

    // TODO: Local/world bounds (oriented bounding boxes)

  public:
    // Cached + write access denied to derived classes

    // TODO: world transform/bounds are calculated on the parent component
    // TODO: when a component's transforms change, all children's world transform are updated
    // -> this way world transforms are always up to date

    /// @param callback: whether to trigger the callback to the component (TODO: ?)
    /// @brief Calculate the world transform according to the parent's component world transform and our own local one.
    /// @note Careful here. I think this version (with an option on "no callback") should be restricted to entity or something
    void CalculateWorldTransform(bool callback = true);

    bool HasSocket(const core::UUID& socketID);

    /// @brief Attach this component to another one.
    /// @param pParentComponent: The component to attach to.
    /// @param socketId: TODO
    /// @todo Document Attached/Detached state
    void AttachTo(SpatialComponent* pParentComponent, const core::UUID& socketID = core::UUID::InvalidID);

    /// @brief Detach this component from its parent.
    void Detach();

    /// @brief Get the local transform of this component.
    const Transform& GetLocalTransform() const { return m_localTransform; }

    /// @brief Get the world transform of this component.
    const Transform& GetWorldTransform() const { return m_worldTransform; }

    /// @brief Set the local transform of this component. Will also update the world positions of all children.
    virtual void SetLocalTransform(Transform transform)
    {
        m_localTransform = transform;
        CalculateWorldTransform(true);
    }
};