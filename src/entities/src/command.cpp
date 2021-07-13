#include "command.hpp"
#include "component.hpp"
#include "entity.hpp"
#include "loading_context.hpp"

namespace aln::entities
{
template <typename T>
using TypeInfo = aln::utils::TypeInfo<T>;

void Command::Execute(Entity* pEntity)
{
    // Read the deferredActions, probably store them somewhere
    // Asynchronously execute them

    // When should the updates occur ? Probably sometime inbetween frames
    // TODO: Where does the context come from ?

    // Example for one:
    LoadingContext context; // TODO
    for (auto action : pEntity->m_deferredActions)
    {
        switch (action.m_type)
        {
        case EntityInternalStateAction::Type::AddComponent:
        {
            auto pParentComponent = pEntity->GetSpatialComponent(action.m_ID);
            pEntity->AddComponentDeferred(context, (IComponent*) action.m_ptr, pParentComponent);
        }
        break;
        case EntityInternalStateAction::Type::CreateSystem:
            pEntity->CreateSystemDeferred(context, (TypeInfo<IEntitySystem>*) action.m_ptr);
            break;
        case EntityInternalStateAction::Type::DestroyComponent:
            pEntity->DestroyComponentDeferred(context, (IComponent*) action.m_ptr);
            break;
        case EntityInternalStateAction::Type::DestroySystem:
            pEntity->DestroySystemDeferred(context, (TypeInfo<IEntitySystem>*) action.m_ptr);
            break;
        default:
            throw std::runtime_error("Unsupported operation");
        }
        // Remove the action from the list if it has been executed successfully
    }
}
} // namespace aln::entities