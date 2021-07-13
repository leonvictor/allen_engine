#pragma once

#include <assert.h>

#include "../update_stages.hpp"
#include <utils/type_info.hpp>

#include "object_model.hpp"
#include "world_update.hpp"

#include <map>
#include <string>
#include <vector>

namespace aln::entities
{
//fwd
class Entity;
class IComponent;

namespace impl
{

using aln::utils::TypeInfo;

/// @brief Abstract base class for systems operating on entities. Systems are singletons (only one of each type associated to an entity).
/// Systems are added to a single entity an can only operate on its components.
class IEntitySystem
{
    friend Entity;
    friend class TypeInfo<IEntitySystem>;

  private:
    /// @brief Return the reflection type of T.
    template <typename T>
    static TypeInfo<IEntitySystem>* GetStaticTypeInfo()
    {
        return TypeInfo<IEntitySystem>::GetTypeInfo<T>().get();
    }

    /// @brief Return the reflection type of this system.
    TypeInfo<IEntitySystem>* GetTypeInfo()
    {
        return m_pTypeInfo;
    }

    TypeInfo<IEntitySystem>* m_pTypeInfo;

  protected:
    /// Stages during which the system should be updated with associated priority scores.
    UpdatePriorities m_requiredUpdatePriorities;
    std::string m_name;

    virtual void Update(UpdateContext const& context){};

    /// @brief Register a component with this system.
    virtual void RegisterComponent(IComponent* pComponent) = 0;

    /// @brief Unregister a component.
    virtual void UnregisterComponent(IComponent* pComponent) = 0;

    /// @brief Returns the update priorities for this system.
    /// @todo https://www.youtube.com/watch?v=jjEsB611kxs 1:47:34 . KRG has a static IEntitySystem::PriorityList attribute
    UpdatePriorities GetRequiredUpdatePriorities() const { return m_requiredUpdatePriorities; }

    // TODO: Explicit Components dependencies (like in "i need a mesh component" to function)
    // -> this should happen in the inherited system classes
    // Optional / Required components
    // Display this to the user
  public:
    std::string GetName() const { return m_name; }
};
} // namespace impl
using impl::IEntitySystem;
} // namespace aln::entities