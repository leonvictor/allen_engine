#include "entity_collection.hpp"
#include "entity.hpp"

std::map<core::UUID, Entity> EntityCollection::m_collection;
std::map<core::UUID, Entity> EntityCollection::m_newlyCreated;

Entity* EntityCollection::Create()
{
    // TODO: This uses the move constructor so we construct the object twice
    Entity entity;
    auto [it, value] = m_newlyCreated.insert(std::make_pair(entity.GetID(), entity));
    // auto [it, value] = EntityCollection::m_newlyCreated.emplace(Entity());

    // TODO: Test this in depth, it's kinda black magic
    // C++17 (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/n4659.pdf):
    // All Associative Containers: The erase members shall invalidate only iterators and references to the erased elements [26.2.6/9]
    // The insert and emplace members shall not affect the validity of iterators and references to the container [26.2.6/9]
    return &(it->second);
}

void EntityCollection::RemoveEntity(const core::UUID& id)
{
    auto it = m_collection.find(id);

    assert(it != m_collection.end());

    m_collection.erase(it);
}