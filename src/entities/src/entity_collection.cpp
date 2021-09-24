#include "entity_collection.hpp"
#include "entity.hpp"

namespace aln::entities
{

std::list<Entity>& EntityCollection::Collection()
{
    static std::list<Entity> collection;
    return collection;
}

void EntityCollection::RemoveEntity(const UUID& id)
{
    Collection().remove_if([id](Entity& entity)
        { return entity.GetID() == id; });
}

void EntityCollection::Clear()
{
    Collection().clear();
}

} // namespace aln::entities
