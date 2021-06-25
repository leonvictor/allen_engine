#include "spatial_component.hpp"

#include <algorithm>
#include <stdexcept>

void SpatialComponent::CalculateWorldTransform(bool callback)
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

bool SpatialComponent::HasSocket(const core::UUID& socketID)
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

void SpatialComponent::AttachTo(SpatialComponent* pParentComponent, const core::UUID& socketID)
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

void SpatialComponent::Detach()
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