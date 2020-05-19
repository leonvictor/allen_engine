#pragma once

#include <unordered_map>
#include <memory>
#include "common.cpp"
#include "component_array.hpp"

namespace ecs
{
class ComponentManager
{
public:
    template <typename T>
    void registerComponent()
    {
        const char *typeName = typeid(T).name();

        assert(mComponentTypes.find(typeName) == mComponentTypes.end() && "Registering component type more than once.");

        // Add this component type to the component type map
        mComponentTypes.insert({typeName, mNextComponentType});

        // Create a ComponentArray pointer and add it to the component arrays map
        mComponentArrays.insert({typeName, std::make_shared<ComponentArray<T>>()});

        // Increment the value so that the next component registered will be different
        ++mNextComponentType;
    }

    template <typename T>
    ComponentType getComponentType()
    {
        const char *typeName = typeid(T).name();

        assert(mComponentTypes.find(typeName) != mComponentTypes.end() && "Component not registered before use.");

        // Return this component's type - used for creating signatures
        return mComponentTypes[typeName];
    }

    template <typename T>
    void addComponent(Entity entity, T component)
    {
        // Add a component to the array for an entity
        getComponentArray<T>()->insertData(entity, component);
    }

    template <typename T>
    void removeComponent(Entity entity)
    {
        // Remove a component from the array for an entity
        getComponentArray<T>()->removeData(entity);
    }

    template <typename T>
    T &getComponent(Entity entity)
    {
        // Get a reference to a component from the array for an entity
        return getComponentArray<T>()->getData(entity);
    }

    void entityDestroyed(Entity entity)
    {
        // Notify each component array that an entity has been destroyed
        // If it has a component for that entity, it will remove it
        for (auto const &pair : mComponentArrays)
        {
            auto const &component = pair.second;

            component->entityDestroyed(entity);
        }
    }

private:
    // Map from type string pointer to a component type
    std::unordered_map<const char *, ComponentType> mComponentTypes{};

    // Map from type string pointer to a component array
    std::unordered_map<const char *, std::shared_ptr<IComponentArray>> mComponentArrays{};

    // The component type to be assigned to the next registered component - starting at 0
    ComponentType mNextComponentType{};

    // Convenience function to get the statically casted pointer to the ComponentArray of type T.
    template <typename T>
    std::shared_ptr<ComponentArray<T>> getComponentArray()
    {
        const char *typeName = typeid(T).name();

        assert(mComponentTypes.find(typeName) != mComponentTypes.end() && "Component not registered before use.");

        return std::static_pointer_cast<ComponentArray<T>>(mComponentArrays[typeName]);
    }
};

} // namespace ecs