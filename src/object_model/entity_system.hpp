#pragma once

#include <assert.h>

#include "../update_stages.hpp"
#include "../utils/type_info.hpp"
#include "object_model.hpp"

#include <map>
#include <vector>

/// @brief Represents the stages during which a system should be updated, as well as the system's priority in each stage.
struct UpdatePriorities
{
    std::unordered_map<UpdateStage, uint8_t> m_updatePriorityMap;

    /// @brief Whether the provided stage is enabled.
    bool IsUpdateStageEnabled(const UpdateStage& stage) const
    {
        return m_updatePriorityMap.count(stage) == 1;
    }

    /// @brief Returns the priority for the provided stage. Making sure the stage is enabled is the user's responsibility.
    uint8_t GetPriorityForStage(const UpdateStage& stage)
    {
        assert(IsUpdateStageEnabled(stage));
        return m_updatePriorityMap[stage];
    }

    // TODO: allow systems to add and (maybe) update their priorities
};

class IComponent;

/// @brief Abstract base class for systems operating on entities. Systems are singletons (only one of each type associated to an entity).
/// Systems are added to a single entity an can only operate on its components.
class IEntitySystem
{
    friend class Entity;

  protected:
    /// Stages during which the system should be updated with associated priority scores.
    UpdatePriorities m_requiredUpdatePriorities;
    /// Components registered with this system
    std::vector<IComponent*> m_components;
    // std::shared_ptr<TypeInfo<IEntitySystem>> m_pTypeInfo;

    std::string m_name;

    virtual void Update(ObjectModel::UpdateContext const& context) = 0;

    /// @brief Register a component with this system.
    virtual void RegisterComponent(IComponent* pComponent);

    /// @brief Unregister a component.
    virtual void UnregisterComponent(IComponent* pComponent);

    /// @brief Returns the update priorities for this system.
    /// @todo https://www.youtube.com/watch?v=jjEsB611kxs 1:47:34 . KRG has a static IEntitySystem::PriorityList attribute
    UpdatePriorities GetRequiredUpdatePriorities() const { return m_requiredUpdatePriorities; }

    // TODO: Explicit Components dependencies (like in "i need a mesh component" to function)
    // -> this should happen in the inherited system classes
    // Optional / Required components
    // Display this to the user
    // TODO: prevent the use of the base class + enforce redefinining of Register/Unregister methods
  public:
    static std::shared_ptr<TypeInfo<IEntitySystem>> StaticTypeInfo;

    virtual ~IEntitySystem() = 0;
    std::string GetName() const { return m_name; }

    /// @brief Return the reflection type of this system.
    virtual TypeInfo<IEntitySystem>* GetTypeInfo() = 0;
    // static TypeInfo<IEntitySystem>* GetTypeInfo()
    // {
    //     // TODO: This won't work because EntitySytem are not designed to be used as is
    //     // TODO: Force derived class to implement this function.
    //     return TypeInfo<IEntitySystem>::GetTypeInfo<IEntitySystem>().get();
    // }
};
