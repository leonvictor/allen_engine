#include "spatial_component.hpp"

#include <algorithm>
#include <stdexcept>

namespace aln::entities
{
using aln::utils::UUID;

void SpatialComponent::CalculateWorldTransform(bool callback)
{
    if (m_pSpatialParent == nullptr)
    {
        // This is the root
        m_worldTransform = m_localTransform;
    }
    else
    {
        // Calculate world transform from the parent's one
        Transform parent = m_pSpatialParent->GetWorldTransform();
        m_worldTransform.position = m_localTransform.position + parent.position;
        m_worldTransform.rotation = m_localTransform.rotation * parent.rotation; // TODO: is that the correct order ?
        m_worldTransform.scale = m_localTransform.scale * parent.scale;
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

bool SpatialComponent::HasSocket(const UUID& socketID)
{
    // TODO: Does this work ?
    // return std::any_of(m_sockets.begin(), m_sockets.end(), socketID);
    return std::any_of(m_sockets.begin(), m_sockets.end(), [socketID](UUID id)
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

void SpatialComponent::AttachTo(SpatialComponent* pParentComponent, const UUID& socketID)
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
    m_parentAttachmentSocketID = UUID::InvalidID();
}
} // namespace aln::entities