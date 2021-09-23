#pragma once

#include <memory>

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
    std::vector<std::unique_ptr<entities::IComponent>> m_registry;

  public:
    entities::ComponentCreationContext context;

    entities::IComponent* Create(const reflect::TypeDescriptor* typeDescriptor)
    {
        // TODO: Alternatively allocate from a component pool
        auto comp = typeDescriptor->typeHelper->CreateType<entities::IComponent>();
        comp->Construct(context);
        auto& record = m_registry.emplace_back(std::move(comp));
        return record.get();
    }

    template <typename T>
    T* Create()
    {
        return (T*) Create(T::GetStaticType());
    }
};
} // namespace aln