#include "entity_collection.hpp"
#include "entity.hpp"

std::set<Entity> EntityCollection::m_collection;
std::set<Entity> EntityCollection::m_newlyCreated;

Entity* EntityCollection::Create()
{
    // TODO: This uses the move constructor so we construct the object twice
    auto [it, value] = EntityCollection::m_newlyCreated.emplace(Entity());

    // TODO: Test this in depth, it's kinda black magic
    // C++17 (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/n4659.pdf):
    // All Associative Containers: The erase members shall invalidate only iterators and references to the erased elements [26.2.6/9]
    // The insert and emplace members shall not affect the validity of iterators and references to the container [26.2.6/9]
    // Entity* const en = *it;
    return const_cast<Entity*>(&(*it));
}

void EntityCollection::RemoveEntity(const core::UUID& id)
{
    auto it = std::find_if(m_collection.begin(), m_collection.end(), [&](auto entity)
        { return entity.GetID() == id; });

    assert(it != m_collection.end());

    m_collection.erase(it);
}