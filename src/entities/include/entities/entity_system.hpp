#pragma once

#include <assert.h>

#include "update_context.hpp"
#include "world_update.hpp"

#include <common/update_stages.hpp>
#include <reflection/reflected_type.hpp>

#include <map>
#include <string>


namespace aln
{
// fwd
class Entity;
class IComponent;

/// @brief Abstract base class for systems operating on entities. Systems are singletons (only one of each type associated to an entity).
/// Systems are added to a single entity an can only operate on its components.
class IEntitySystem : public reflect::IReflected
{
    friend Entity;

  protected:
    /// Stages during which the system should be updated with associated priority scores.
    UpdatePriorities m_requiredUpdatePriorities;
    std::string m_name;

    virtual void Update(const UpdateContext& context) = 0;

    /// @brief Register a component with this system.
    virtual void RegisterComponent(IComponent* pComponent) = 0;

    /// @brief Unregister a component.
    virtual void UnregisterComponent(IComponent* pComponent) = 0;

    /// @brief Returns the update priorities for this system.
    /// @todo https://www.youtube.com/watch?v=jjEsB611kxs 1:47:34 . KRG has a static IEntitySystem::PriorityList attribute
    const UpdatePriorities& GetRequiredUpdatePriorities() const { return m_requiredUpdatePriorities; }

    // TODO: Explicit Components dependencies (like in "i need a mesh component" to function)
    // -> this should happen in the inherited system classes
    // Optional / Required components
    // Display this to the user
  public:
    virtual ~IEntitySystem() {}
    std::string GetName() const { return m_name; }
};
} // namespace aln