#include "../transform.hpp"
#include "../utils/uuid.cpp"
#include "component.cpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

/// @brief Entities with a spatial component have a position and orientation in the world.
/// They can be attached to other spatial entities to form hierarchies.
class SpatialComponent : public Component
{

  private:
    friend class Entity;

    /// @brief List of attached children components.
    std::vector<SpatialComponent*> m_spatialChildren;

    /// @brief Parent component.
    SpatialComponent* m_pSpatialParent;
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
    void CalculateWorldTransform(bool callback = true)
    {
        if (m_pSpatialParent == nullptr)
        {
            // This is the root
            m_worldTransform = m_localTransform;
        }
        else
        {
            // TODO: Calculate world transform from the parent's one
            throw std::runtime_error("Not implemented");
        }
        if (callback)
        {
            // Update the world transform of all children recursively
            for (auto* pChild : m_spatialChildren)
            {
                pChild->CalculateWorldTransform();
            }
        }
    }

    bool HasSocket(const core::UUID& socketID)
    {
        // TODO: Does this work ?
        // return std::any_of(m_sockets.begin(), m_sockets.end(), socketID);
        return std::any_of(m_sockets.begin(), m_sockets.end(), [socketID](core::UUID id)
                           { return id == socketID; });
        // for (StringID id : m_sockets)
        // {
        //     if (id == socketID)
        //     {
        //         return true;
        //     }
        // }
        // return false;
    }

    /// @brief Attach this component to another one.
    /// @param pParentComponent: The component to attach to.
    /// @param socketId: TODO
    /// @todo Document Attached/Detached state
    void AttachTo(SpatialComponent* pParentComponent, const core::UUID& socketID = core::UUID::InvalidID)
    {
        // TODO: Handle sockets
        // We can't attach if we're already attached
        assert(m_pSpatialParent == nullptr && pParentComponent != nullptr);

        // Set component hierarchy values
        m_pSpatialParent = pParentComponent;
        m_parentAttachmentSocketID = socketID; // TODO: actually handle this...
        // TODO: should we calculate world transform everytime a component is attached to another ?
        CalculateWorldTransform();

        // Add to the list of child components on the component to attach to
        pParentComponent->m_spatialChildren.emplace_back(this);
    }

    /// @brief Detach this component from its parent.
    void Detach()
    {
        // TODO: Handle sockets
        assert(m_pSpatialParent != nullptr);

        // Remove from parent component child list
        auto foundIter = std::find(m_pSpatialParent->m_spatialChildren.begin(), m_pSpatialParent->m_spatialChildren.end(), this);
        assert(foundIter != m_pSpatialParent->m_spatialChildren.end());
        m_pSpatialParent->m_spatialChildren.erase(foundIter);

        // Remove component hierarchy values
        m_pSpatialParent = nullptr;
        m_parentAttachmentSocketID = core::UUID::InvalidID;
    }

    // // TODO: ReadOnly AND modifiable getters for transform, so we can pick which one to
    // const Transform& GetLocalTransformReadOnly() const
    // {
    //     return m_localTransform;
    // }

    // Transform& GetLocalTransform()
    // {
    //     return m_localTransform;
    //     // Doesn't work, we can't update here cause the client wont have finish modifications
    // }
};