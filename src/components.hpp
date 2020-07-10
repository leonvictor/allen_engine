#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

class Component
{
};
class Entity
{
  public:
    // TODO: Map from component class type name (str) to actual component.
    std::unordered_map<std::string, std::shared_ptr<Component>> components_;

    // Component methods
    template <typename T>
    void addComponent(std::shared_ptr<T> component)
    {
        std::string typeName = typeid(T).name();
        if (components_.find(typeName) != components_.end())
        {
            throw std::runtime_error("Object already has a component of type " + typeName);
        }

        components_.insert(std::pair<std::string, std::shared_ptr<Component>>(typeName, component));
    }

    // Retrieve a component from this Entity.
    // Usage: MyComponent comp = myEntity.getComponent<MyComponent>();
    // For now, each Entity can have only one Component of each type.
    template <typename T>
    std::shared_ptr<T> getComponent()
    {
        std::string typeName = typeid(T).name();
        return std::static_pointer_cast<T>(components_[typeName]);
    }
};