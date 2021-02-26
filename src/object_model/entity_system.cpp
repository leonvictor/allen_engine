#pragma once

#include <assert.h>

#include "../utils/type_info.hpp"
#include "object_model.cpp"
#include <unordered_map>
#include <vector>

// TODO: This is engine-wide. Should be used by the main game loop
// TODO: having numstages this way could be a bit wonky
enum UpdateStage
{
    FrameStart,
    PrePhysics,
    Physics,
    PostPhysics,
    FrameEnd,
    NumStages
};

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

// Systems are singletons (only one of each type in an entity)
class EntitySystem
{
  private:
    UpdatePriorities m_requiredUpdatePriorities;
    std::vector<Component*> m_components;
    std::shared_ptr<TypeInfo<EntitySystem>> m_pTypeInfo;

    // TODO: Explicit Components dependencies (like in "i need a mesh component" to function)
    // -> this should happen in the inherited system classes
    // TODO: prevent the use of the base class + enforce redefinining of Register/Unregister methods
  public:
    static std::shared_ptr<TypeInfo<EntitySystem>> StaticTypeInfo;

    EntitySystem() {}

    void Update(ObjectModel::UpdateContext const& context) {}

    void RegisterComponent(Component* pComponent)
    {
        assert(pComponent != nullptr);
        // todo: this is where we check if the furnished component is expected
        // todo: cast as correct component type
        // todo if not nullptr

        // todo: replace with a map <entityId;pComponent>
        m_components.push_back(pComponent);
    }

    void UnregisterComponent(Component* pComponent)
    {
        // todo: accept entity id and get the record from the map
        // if not nullptr
        //    also remove further dependencies
        // remove from map
    }

    /// @brief Returns the update priorities for this component.
    UpdatePriorities GetRequiredUpdatePriorities() const { return m_requiredUpdatePriorities; }

    static TypeInfo<EntitySystem>* GetTypeInfo()
    {
        // TODO: This won't work because EntitySytem are not designed to be used as is
        // TODO: Force derived class to implement this function.
        return TypeInfo<EntitySystem>::GetTypeInfo<EntitySystem>().get();
    }
};
