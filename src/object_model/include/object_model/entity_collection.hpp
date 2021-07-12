#pragma once

#include <map>
#include <utils/uuid.hpp>

class Entity;
namespace core
{
class UUID;
}

/// @brief Singleton holder of *all* the entities.
// TODO: Should be responsible for ensuring concurrency
// TODO: maybe use shared_ptr ? And make sure this is the last place an entity is destroyed.
class EntityCollection
{
  private:
    friend class Entity;

  protected:
    static std::map<core::UUID, Entity> m_collection;
    /// @note I think it'd be good if newlyCreated existed separately in each thread.
    /// Then we could concatenate at some point. Maybe all execution thread acquires a separate EntityMap object,
    /// and the "main" (not transient) one syncs up once per frame ?
    static std::map<core::UUID, Entity> m_newlyCreated;

    /// @brief Create a new entity, insert it in the collection and return it.
    /// @todo Ensure this works even if called from different threads
    ///       OR make sure we only call this from the main thread ?
    /// @note std::sets are not thread-safe.
    ///       Then what might be a good solution ? Lock in this function (meh) ?
    ///       Maybe allow each separate thread to acquire an empty version of each vector, then concatenate them at the sync point ?
    static Entity* Create();

    /// @brief Definitely remove an entity from the collection.
    static void RemoveEntity(const core::UUID& id);
};