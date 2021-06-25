#include "entity_system.hpp"

#include "component.hpp"
#include "object_model.hpp"

#include <assert.h>
#include <map>
#include <vector>

class IComponent;

/// @brief Register a component with this system.
void IEntitySystem::RegisterComponent(IComponent* pComponent)
{
    assert(pComponent != nullptr);
    // todo: this is where we check if the furnished component is expected
    // todo: cast as correct component type
    // todo if not nullptr

    m_components.push_back(pComponent);
}

/// @brief Unregister the components of an entity from this system.
void IEntitySystem::UnregisterComponent(IComponent* pComponent)
{
    auto it = std::find(m_components.begin(), m_components.end(), pComponent);
    if (it != m_components.end())
    {
        m_components.erase(it);
    }
    // if not nullptr
    //    also remove further dependencies
    // remove from map
}
