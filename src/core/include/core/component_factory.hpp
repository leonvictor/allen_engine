#pragma once

#include <entities/component.hpp>
#include <entities/component_creation_context.hpp>

#include <reflection/reflection.hpp>

#include "Tracy.hpp"

namespace aln
{

namespace vkg
{
class Device;
}

class ComponentFactory
{
  public:
    entities::ComponentCreationContext context;

    entities::IComponent* Create(const reflect::TypeDescriptor* typeDescriptor)
    {
        // TODO: Alternatively allocate from a component pool
        auto comp = (entities::IComponent*) typeDescriptor->typeHelper->CreateType();
        comp->Construct(context);
        return comp;
    }

    template <typename T>
    T* Create()
    {
        ZoneScoped;

        return (T*) Create(T::GetStaticType());
    }
};
} // namespace aln