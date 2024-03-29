#include "spatial_component.hpp"

#include <common/maths/angles.hpp>

#include <algorithm>
#include <stdexcept>

namespace aln
{

ALN_REGISTER_ABSTRACT_IMPL_BEGIN(SpatialComponent)
ALN_REFLECT_MEMBER(m_localTransform)
ALN_REGISTER_IMPL_END()

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
        m_worldTransform.SetTranslation(parent.GetTranslation() + (parent.GetRotation().RotateVector(m_localTransform.GetTranslation())));
        m_worldTransform.SetRotation(parent.GetRotation() * m_localTransform.GetRotation());
        m_worldTransform.SetScale(m_localTransform.GetScale().Scale(parent.GetScale()));
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

    // Offset the current local transform so that the world transform stay identical when parent is changed
    auto parentTransform = pParentComponent->GetWorldTransform();
    m_localTransform.SetScale(m_localTransform.GetScale().Scale(1.0f / parentTransform.GetScale()));
    m_localTransform.SetTranslation((parentTransform.GetRotation().Conjugated().RotateVector(m_localTransform.GetTranslation())) - parentTransform.GetTranslation());
    m_localTransform.SetRotation(parentTransform.GetRotation().Conjugated() * m_localTransform.GetRotation());

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
    m_parentAttachmentSocketID = UUID::InvalidID;

    // Offset local transform to account for the old parent's one
    m_localTransform = m_worldTransform;
}

void SpatialComponent::SetLocalTransformRotation(const Quaternion& quat)
{
    m_localTransform.SetRotation(quat);
    CalculateWorldTransform(true);
}

void SpatialComponent::SetLocalTransformRotationEuler(const EulerAnglesDegrees& euler)
{
    m_localTransform.SetRotationEuler(euler);
    CalculateWorldTransform(true);
}

void SpatialComponent::SetLocalTransformPosition(const Vec3& pos)
{
    m_localTransform.SetTranslation(pos);
    CalculateWorldTransform(true);
}

void SpatialComponent::SetLocalTransformScale(const Vec3& scale)
{
    m_localTransform.SetScale(scale);
    CalculateWorldTransform(true);
}

void SpatialComponent::SetLocalTransformPositionAndRotation(const Vec3& pos, const Quaternion& rotation)
{
    m_localTransform.SetRotation(rotation);
    m_localTransform.SetTranslation(pos);
    CalculateWorldTransform(true);
}

void SpatialComponent::OffsetLocalTransformPosition(const Vec3& offset)
{
    m_localTransform.SetTranslation(m_localTransform.GetTranslation() + offset);
    CalculateWorldTransform(true);
}

void SpatialComponent::OffsetLocalTransformRotation(const Quaternion& quatOffset)
{
    auto rot = (m_localTransform.GetRotation() * quatOffset).Normalized();
    m_localTransform.SetRotation(rot);
    CalculateWorldTransform(true);
}
} // namespace aln