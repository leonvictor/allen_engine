#pragma once

#include "component.hpp"
#include "transform.hpp"

#include <utils/uuid.hpp>

#include <vector>

/// @brief Wrapper around an object calling a suffix method when it's destroyed.
/// @note Adapted from https://stroustrup.com/wrapper.pdf
template <class T, class Suf>
class Call_proxy
{
    T* p;
    Suf suffix;

    mutable bool accessed;
    mutable T copy;

    Call_proxy& operator=(const Call_proxy&); // prevent assignment
    Call_proxy(const Call_proxy&);            // prevent copy constructor

  public:
    template <class U, class P, class S>
    friend class Wrap;

    Call_proxy(T* pp, Suf su) : p(pp), suffix(su), accessed(false) {}
    Call_proxy(T& x, Suf su) : p(&x), suffix(su), accessed(false) {}

    ~Call_proxy()
    {
        if (accessed && copy != *p)
            suffix();
    }

    T* operator->() const
    {
        accessed = true;
        copy = *p;
        return p;
    }
};

typedef Call_proxy<Transform, std::function<void()>> ModifiableTransform;

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

    /// @param callback: whether to trigger the callback to calculate the component's children's world transform.
    /// @brief Calculate the world transform according to the parent's component world transform and our own local one.
    void CalculateWorldTransform(bool callback = true);

    /// @brief Internal method called when the local transform is modified.
    void TransformUpdateCallback()
    {
        AfterTransformUpdate();        // Overloadable segment
        CalculateWorldTransform(true); // Fixed one
    }

  protected:
    /// @brief Overloadable function to specify operations that should happen every time the component's transform is updated.
    virtual void AfterTransformUpdate() {}

  public:
    // Cached + write access denied to derived classes
    // TODO: world transform/bounds are calculated on the parent component

    bool HasSocket(const core::UUID& socketID);

    /// @brief Attach this component to another one.
    /// @param pParentComponent: The component to attach to.
    /// @param socketId: TODO
    /// @todo Document Attached/Detached state
    void AttachTo(SpatialComponent* pParentComponent, const core::UUID& socketID = core::UUID::InvalidID);

    /// @brief Detach this component from its parent.
    void Detach();

    /// @brief Get a modifiable pointer to this component's local transform. Using this accessor comes at a slight performance cost,
    /// prefer using the readonly version GetLocalTransform when possible.
    /// @todo Profile. The wrapper makes short-lived copies of the transform every time it is accessed, and compares it to the new one when
    /// it is destroyed.
    /// @todo The full hierarchy's world transforms are computed every time a local transform is modified. This is alright for now but might be problematic later on.
    /// It might be a good idea to flag the modified transform, and only compute the world ones when we need them.
    inline const ModifiableTransform ModifyTransform()
    {
        return ModifiableTransform(m_localTransform, std::bind(&SpatialComponent::TransformUpdateCallback, this));
    }

    /// @brief Get the local transform of this component.
    const Transform& GetLocalTransform()
    {
        return m_localTransform;
    }

    /// @brief Get the world transform of this component.
    const Transform& GetWorldTransform() const { return m_worldTransform; }

    /// @brief Set the local transform of this component. Will also update the world positions of all children.
    virtual void SetLocalTransform(Transform& transform)
    {
        m_localTransform = transform;
        CalculateWorldTransform(true);
        AfterTransformUpdate();
    }
};