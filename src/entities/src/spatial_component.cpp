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
        m_worldTransform.m_position = parent.m_position + (parent.m_rotation * m_localTransform.m_position);
        m_worldTransform.m_rotation = parent.m_rotation * m_localTransform.m_rotation;
        m_worldTransform.m_scale = m_localTransform.m_scale * parent.m_scale;
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
    // Offset the current local transform so that the object doesn't move when parent is changed
    auto parentTransform = pParentComponent->GetWorldTransform();
    m_localTransform.m_position = m_localTransform.m_position - parentTransform.m_position;
    // m_localTransform.m_rotation = glm::conjugate(parentTransform.m_rotation) * m_localTransform.m_rotation; // TODO: is that the correct order ?
    m_localTransform.m_scale = m_localTransform.m_scale / parentTransform.m_scale;
    CalculateWorldTransform();

    // Add to the list of child components on the component to attach to
    pParentComponent->m_spatialChildren.push_back(this);
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

    // Fix the world transform to not depend on parent
    m_localTransform = m_worldTransform;
}

void SpatialComponent::SetLocalTransformRotation(const glm::quat quat)
{
    m_localTransform.m_rotation = quat;
    auto ypr = glm::degrees(glm::eulerAngles(m_localTransform.m_rotation));
    m_localTransform.m_rotationEuler = glm::vec3(ypr.y, ypr.x, ypr.z);
    CalculateWorldTransform(true);
}

void SpatialComponent::SetLocalTransformRotationEuler(const glm::vec3 euler)
{
    m_localTransform.m_rotationEuler = euler;
    m_localTransform.m_rotation = glm::quat(glm::radians(euler));
    CalculateWorldTransform(true);
}

void SpatialComponent::SetLocalTransformPosition(const glm::vec3 pos)
{
    m_localTransform.m_position = pos;
    CalculateWorldTransform(true);
}

void SpatialComponent::SetLocalTransformScale(const glm::vec3 scale)
{
    m_localTransform.m_scale = scale;
    CalculateWorldTransform(true);
}

void SpatialComponent::OffsetLocalTransformPosition(const glm::vec3 offset)
{
    m_localTransform.m_position += offset;
    CalculateWorldTransform(true);
}

void SpatialComponent::OffsetLocalTransformRotation(const glm::quat quatOffset)
{
    m_localTransform.m_rotation += quatOffset;
    auto ypr = glm::degrees(glm::eulerAngles(m_localTransform.m_rotation));
    m_localTransform.m_rotationEuler = glm::vec3(ypr.y, ypr.x, ypr.z);
    CalculateWorldTransform(true);
}
} // namespace aln::entities