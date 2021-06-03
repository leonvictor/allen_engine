#pragma once

#include <assert.h>

#include "../update_stages.hpp"
#include "../utils/type_info.hpp"
#include "object_model.cpp"

#include <unordered_map>
#include <vector>

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

class Component;

/// @brief Abstract base class for systems operating on entities. Systems are singletons (only one of each type associated to an entity).
class IEntitySystem
{
  private:
    UpdatePriorities m_requiredUpdatePriorities;
    /// Components registered with this system
    std::vector<Component*> m_components;
    std::shared_ptr<TypeInfo<IEntitySystem>> m_pTypeInfo;

    // TODO: Explicit Components dependencies (like in "i need a mesh component" to function)
    // -> this should happen in the inherited system classes
    // TODO: prevent the use of the base class + enforce redefinining of Register/Unregister methods
  public:
    static std::shared_ptr<TypeInfo<IEntitySystem>> StaticTypeInfo;

    IEntitySystem() {}

    virtual void Update(ObjectModel::UpdateContext const& context) = 0;

    /// @brief Register a component with this system.
    void RegisterComponent(Component* pComponent)
    {
        assert(pComponent != nullptr);
        // todo: this is where we check if the furnished component is expected
        // todo: cast as correct component type
        // todo if not nullptr

        // todo: replace with a map <entityId;pComponent>
        m_components.push_back(pComponent);
    }

    /// @brief Unregister a component with this system.
    void UnregisterComponent(Component* pComponent)
    {
        // todo: accept entity id and get the record from the map
        // if not nullptr
        //    also remove further dependencies
        // remove from map
    }

    /// @brief Returns the update priorities for this system.
    UpdatePriorities GetRequiredUpdatePriorities() const { return m_requiredUpdatePriorities; }

    /// @brief Return the reflection type of this system.
    static TypeInfo<IEntitySystem>* GetTypeInfo()
    {
        // TODO: This won't work because EntitySytem are not designed to be used as is
        // TODO: Force derived class to implement this function.
        return TypeInfo<IEntitySystem>::GetTypeInfo<IEntitySystem>().get();
    }
};
