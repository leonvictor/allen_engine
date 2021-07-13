#include "entity_collection.hpp"
#include "entity.hpp"

namespace aln::entities
{

std::map<UUID, Entity>& EntityCollection::Collection()
{
    static std::map<UUID, Entity> collection;
    return collection;
}

std::map<UUID, Entity>& EntityCollection::NewlyCreatedEntities()
{
    static std::map<UUID, Entity> collection;
    return collection;
}

Entity* EntityCollection::Create()
{
    // TODO: This uses the move constructor so we construct the object twice
    Entity entity;

    auto& newlyCreated = NewlyCreatedEntities();
    auto [it, value] = newlyCreated.insert(std::make_pair(entity.GetID(), entity));
    // auto [it, value] = EntityCollection::m_newlyCreated.emplace(Entity());

    // TODO: Test this in depth, it's kinda black magic
    // C++17 (http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/n4659.pdf):
    // All Associative Containers: The erase members shall invalidate only iterators and references to the erased elements [26.2.6/9]
    // The insert and emplace members shall not affect the validity of iterators and references to the container [26.2.6/9]
    return &(it->second);
}

void EntityCollection::RemoveEntity(const UUID& id)
{
    auto& collection = Collection();
    auto it = collection.find(id);

    assert(it != collection.end());

    collection.erase(it);
}

void EntityCollection::Clear()
{
    Collection().clear();
    NewlyCreatedEntities().clear();
}

} // namespace aln::entities
