#include <stdexcept>
#include <utils/ring_buffer.cpp>

#include "entity.hpp"

class Component;
class EntitySystem;
// TODO: Move somewhere where this makes sense
class EventQueue
{
  private:
    RingBuffer<Entity*> m_queue;

  public:
    void Execute(Entity* pEntity)
    {
        // TODO: Aggregate the deferredActions if pEntity has already been queued
        // TODO: Think about the repartition strategy:
        // It might be a good idea to split actions by type so that we don't have to loop over all of them at each stage (Loading, Initializing, etc)
        m_queue.Enqueue(pEntity);
    }

    // TODO: Update should be called by a world system managing entities.
    void Update(ObjectModel::LoadingContext& context)
    {
        // Asynchronously execute the queued event
        // When should the updates occur ? Probably sometime inbetween frames
        // TODO: Where does the context come from ?

        // TODO: For now we're simply looping over all deferred actions
        while (!m_queue.Empty())
        {
            auto pEntity = m_queue.Pop();
            for (auto action : pEntity->m_deferredActions)
            {
                switch (action.m_type)
                {
                case EntityInternalStateAction::Type::AddComponent:
                    // TODO: This would probably be better inside entity
                    auto pParentComponent = pEntity->GetSpatialComponent(action.m_ID);
                    pEntity->AddComponentDeferred(context, (Component*) action.m_ptr, pParentComponent);
                    break;
                case EntityInternalStateAction::Type::CreateSystem:
                    pEntity->CreateSystemDeferred(context, (TypeInfo<EntitySystem>*) action.m_ptr);
                    break;
                case EntityInternalStateAction::Type::DestroyComponent:
                    pEntity->DestroyComponentDeferred(context, (Component*) action.m_ptr);
                    break;
                case EntityInternalStateAction::Type::DestroySystem:
                    pEntity->DestroySystemDeferred(context, (TypeInfo<EntitySystem>*) action.m_ptr);
                    break;
                default:
                    throw std::runtime_error("Unsupported operation");
                }
            }
        }
    }
};