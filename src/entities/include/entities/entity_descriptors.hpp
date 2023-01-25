#pragma once

#include <string>
#include <vector>

#include <common/string_id.hpp>
#include <reflection/type_descriptor.hpp>

namespace aln
{
// We can't use the default serialization system for entities
// because of the dynamic types of components and systems

class ComponentDescriptor : public reflect::TypeDescriptor
{
    friend class EntityDescriptor;

  public:
    template <class Archive>
    void Serialize(Archive& archive) const
    {
        reflect::TypeDescriptor::Serialize(archive);
    }

    template <class Archive>
    void Deserialize(Archive& archive)
    {
        reflect::TypeDescriptor::Deserialize(archive);
    }
};

class SystemDescriptor
{
    friend class EntityDescriptor;
    StringID m_typeID;
};

class EntityDescriptor
{
    friend class Entity;

  private:
    // TODO: Unique identifier, but UUID is not garanteed to stay the same between executions.
    // Use name ? As a StringID maybe ?
    std::string m_name;
    std::vector<ComponentDescriptor> m_componentDescriptors;
    std::vector<SystemDescriptor> m_systemDescriptors;

  public:
    EntityDescriptor() = default;
    EntityDescriptor(const Entity* pEntity, const TypeRegistryService* pTypeRegistryService)
    {
        m_name = pEntity->m_name;
        for (auto pComponent : pEntity->m_components)
        {
            auto& componentDesc = m_componentDescriptors.emplace_back();
            componentDesc.DescribeTypeInstance(pComponent, pTypeRegistryService, pComponent->GetTypeInfo());
            // todo: Spatial info
        }
        for (auto pSystem : pEntity->m_systems)
        {
            auto& systemDesc = m_systemDescriptors.emplace_back();
            systemDesc.m_typeID = pSystem->GetTypeInfo()->GetTypeID();
        }
    }

    Entity* InstanciateEntity(const TypeRegistryService* pTypeRegistryService)
    {
        // TODO: Instanciate in place
        Entity* pEntity = aln::New<Entity>();
        pEntity->m_name = m_name;
        for (auto& componentDesc : m_componentDescriptors)
        {
            auto pComponent = componentDesc.Instanciate<IComponent>(pTypeRegistryService);
            // TODO: Spatial info
            pEntity->AddComponent(pComponent);
        }

        for (auto& systemDesc : m_systemDescriptors)
        {
            auto pSystemTypeInfo = pTypeRegistryService->GetTypeInfo(systemDesc.m_typeID);
            pEntity->CreateSystem(pSystemTypeInfo);
        }

        return pEntity;
    }

  public:
    template <class Archive>
    void Serialize(Archive& archive) const
    {
        archive << m_name;
        archive << m_componentDescriptors;
        archive << m_systemDescriptors;
    }

    template <class Archive>
    void Deserialize(Archive& archive)
    {
        archive >> m_name;
        archive >> m_componentDescriptors;
        archive >> m_systemDescriptors;
    }
};

static_assert(CustomSerializable<EntityDescriptor>);

/// @brief Represents a collection of entities, and can be used to instanciate them all in contiguous memory
class EntityMapDescriptor
{
};

} // namespace aln