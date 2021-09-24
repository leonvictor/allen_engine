#pragma once

#include <list>
#include <utils/uuid.hpp>

namespace aln::entities
{

class Entity;
using aln::utils::UUID;

class EntityCollection
{
  private:
    friend Entity;

  protected:
    static std::list<Entity>& Collection();

    /// @brief Definitely remove an entity from the collection.
    static void RemoveEntity(const UUID& id);

    /// @brief Definitely remove all entities from this collection.
    static void Clear();
};
} // namespace aln::entities