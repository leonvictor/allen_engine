#include "../utils/uuid.cpp"
#include "component.cpp"
#include <algorithm>
#include <stdexcept>
#include <vector>

class SpatialComponent : public Component
{

  private:
    friend class Entity;

    std::vector<SpatialComponent*> m_spatialChildren;
    SpatialComponent* m_pSpatialParent;
    std::vector<UUID> m_sockets;

    // TODO: Handle socket
    UUID m_parentAttachmentSocketID;

  public:
    // TODO: Local transform
    // TODO: World transform
    // TODO: Local/world bounds (oriented bounding boxes)
    // Cached + write access denied to derived classes

    // TODO: world transform/bounds are calculated on the parent component
    // TODO: when a component's transforms change, all children's world transform are updated
    // -> this way world transforms are always up to date

    // TODO: Careful here. I think this version (with an option on "no callback") should be restricted to entity or something
    /// @brief TODO
    /// @param callback: whether to trigger the callback to the component (TODO: ?)
    void CalculateWorldTransform(bool callback = true)
    {
        // TODO
        throw std::runtime_error("Not implemented");
    }

    bool HasSocket(const UUID& socketID)
    {
        // TODO: Does this work ?
        // return std::any_of(m_sockets.begin(), m_sockets.end(), socketID);
        return std::any_of(m_sockets.begin(), m_sockets.end(), [socketID](UUID id) { return id == socketID; });
        // for (StringID id : m_sockets)
        // {
        //     if (id == socketID)
        //     {
        //         return true;
        //     }
        // }
        // return false;
    }

    void AttachTo(SpatialComponent* pParentComponent, const UUID& socketID = UUID::InvalidID)
    {
        // TODO: Handle sockets
        assert(m_pSpatialParent == nullptr && pParentComponent != nullptr);

        // Set component hierarchy values
        m_pSpatialParent = pParentComponent;
        m_parentAttachmentSocketID = socketID; // TODO: actually handle this...
        // TODO: should we calculate world transform everytime a component is attached to another ?
        CalculateWorldTransform();

        // Add to the list of child components on the component to attach to
        pParentComponent->m_spatialChildren.emplace_back(this);
    }

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
        m_parentAttachmentSocketID = UUID::InvalidID;
    }
};