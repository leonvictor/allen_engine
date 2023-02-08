#pragma once

#include <string>
#include <vector>

#include <common/string_id.hpp>
#include <reflection/type_descriptor.hpp>

#include "component.hpp"
#include "entity.hpp"
#include "entity_map.hpp"
#include "spatial_component.hpp"

namespace aln
{
// We can't use the default serialization system for entities
// because of the dynamic types of components and systems

class ComponentDescriptor : public reflect::TypeDescriptor
{
    friend class EntityDescriptor;

  private:
    bool m_isSpatialComponent = false;
    uint32_t m_spatialParentIndex = InvalidIndex;

  public:
    bool IsSpatialComponent() const { return m_isSpatialComponent; }

  public:
    template <class Archive>
    void Serialize(Archive& archive) const
    {
        reflect::TypeDescriptor::Serialize(archive);
        archive << m_isSpatialComponent;
        archive << m_spatialParentIndex;
    }

    template <class Archive>
    void Deserialize(Archive& archive)
    {
        reflect::TypeDescriptor::Deserialize(archive);
        archive >> m_isSpatialComponent;
        archive >> m_spatialParentIndex;
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
    struct SpatialComponentsRelationship
    {
        uint32_t m_componentIndex = InvalidIndex;
        uint32_t m_parentComponentIndex = InvalidIndex;
    };

    static uint32_t GetComponentIndex(const Entity* pEntity, const IComponent* pComponent)
    {
        auto componentCount = pEntity->m_components.size();
        for (uint32_t componentIndex = 0; componentIndex < componentCount; ++componentIndex)
        {
            auto pEntityComponent = pEntity->m_components[componentIndex];
            if (pEntityComponent == pComponent)
            {
                return componentIndex;
            }
        }
        return InvalidIndex;
    }

  private:
    // TODO: Unique identifier, but UUID is not garanteed to stay the same between executions.
    // Use name ? As a StringID maybe ?
    std::string m_name;
    std::vector<ComponentDescriptor> m_componentDescriptors;
    std::vector<SystemDescriptor> m_systemDescriptors;
    std::vector<SpatialComponentsRelationship> m_spatialComponentsRelationships;

  public:
    EntityDescriptor() = default;
    EntityDescriptor(const Entity* pEntity, const TypeRegistryService* pTypeRegistryService)
    {
        assert(pEntity != nullptr);

        m_name = pEntity->m_name;

        uint32_t componentIndex = 0;
        for (auto pComponent : pEntity->m_components)
        {
            auto& componentDesc = m_componentDescriptors.emplace_back();
            componentDesc.DescribeTypeInstance(pComponent, pTypeRegistryService, pComponent->GetTypeInfo());

            auto pSpatialComponent = dynamic_cast<SpatialComponent*>(pComponent);
            if (pSpatialComponent != nullptr)
            {
                componentDesc.m_isSpatialComponent = true;

                auto& relationship = m_spatialComponentsRelationships.emplace_back();
                relationship.m_componentIndex = componentIndex;
                if (pSpatialComponent->m_pSpatialParent != nullptr)
                {
                    relationship.m_parentComponentIndex = GetComponentIndex(pEntity, pSpatialComponent->m_pSpatialParent);
                }
            }

            componentIndex++;
        }

        for (auto pSystem : pEntity->m_systems)
        {
            auto& systemDesc = m_systemDescriptors.emplace_back();
            systemDesc.m_typeID = pSystem->GetTypeInfo()->GetTypeID();
        }
    }

    void InstanciateEntity(Entity* pEntity, const TypeRegistryService* pTypeRegistryService)
    {
        assert(pEntity != nullptr);

        pEntity->m_name = m_name;
        for (auto& componentDesc : m_componentDescriptors)
        {
            auto pComponent = componentDesc.Instanciate<IComponent>(pTypeRegistryService);
            // TODO: Spatial info
            pEntity->AddComponent(pComponent);
        }

        for (auto& relationship : m_spatialComponentsRelationships)
        {
            if (relationship.m_parentComponentIndex != InvalidIndex)
            {
                auto pSpatialComponent = dynamic_cast<SpatialComponent*>(pEntity->m_components[relationship.m_componentIndex]);
                auto pParentSpatialComponent = dynamic_cast<SpatialComponent*>(pEntity->m_components[relationship.m_parentComponentIndex]);

                assert(pSpatialComponent != nullptr && pParentSpatialComponent != nullptr);

                // TODO: We could use attach but it offsets the transform
                pSpatialComponent->m_pSpatialParent = pParentSpatialComponent;
                pParentSpatialComponent->m_spatialChildren.push_back(pSpatialComponent);
            }
        }

        for (auto& systemDesc : m_systemDescriptors)
        {
            auto pSystemTypeInfo = pTypeRegistryService->GetTypeInfo(systemDesc.m_typeID);
            pEntity->CreateSystem(pSystemTypeInfo);
        }
    }

    bool IsValid() const { return !m_name.empty(); }

  public:
    template <class Archive>
    void Serialize(Archive& archive) const
    {
        archive << m_name;
        archive << m_componentDescriptors;
        archive << m_systemDescriptors;
        archive << m_spatialComponentsRelationships;
    }

    template <class Archive>
    void Deserialize(Archive& archive)
    {
        archive >> m_name;
        archive >> m_componentDescriptors;
        archive >> m_systemDescriptors;
        archive >> m_spatialComponentsRelationships;
    }
};

static_assert(CustomSerializable<EntityDescriptor>);

/// @brief Represents a collection of entities, and can be used to instanciate them all in contiguous memory
class EntityMapDescriptor
{
  private:
    struct SpatialEntitiesRelationship
    {
        uint32_t m_entityIndex = InvalidIndex;
        uint32_t m_parentEntityIndex = InvalidIndex;
    };

    static uint32_t GetEntityIndex(const EntityMap& entityMap, const Entity* pEntity)
    {
        auto entityCount = entityMap.m_entities.size();
        for (uint32_t entityIndex = 0; entityIndex < entityCount; ++entityIndex)
        {
            auto pMapEntity = entityMap.m_entities[entityIndex];
            if (pMapEntity == pEntity)
            {
                return entityIndex;
            }
        }
        return InvalidIndex;
    }

  private:
    std::vector<EntityDescriptor> m_entityDescriptors;
    std::vector<SpatialEntitiesRelationship> m_spatialEntitiesRelationships;

  public:
    EntityMapDescriptor() = default;
    EntityMapDescriptor(const EntityMap& entityMap, const TypeRegistryService& typeRegistryService)
    {
        uint32_t entityIndex = 0;
        for (auto pEntity : entityMap.m_entities)
        {
            m_entityDescriptors.emplace_back(pEntity, &typeRegistryService);
            if (pEntity->IsSpatialEntity())
            {
                auto& relationship = m_spatialEntitiesRelationships.emplace_back();
                relationship.m_entityIndex = entityIndex;
                if (pEntity->m_pParentSpatialEntity != nullptr)
                {
                    relationship.m_parentEntityIndex = GetEntityIndex(entityMap, pEntity->m_pParentSpatialEntity);
                }
            }

            ++entityIndex;
        }
    }

    void InstanciateEntityMap(EntityMap& entityMap, const TypeRegistryService& typeRegistryService)
    {
        auto entityCount = m_entityDescriptors.size();
        entityMap.m_entities.reserve(entityCount);
        entityMap.m_loadingEntities.reserve(entityCount);

        // TODO: Parallelize ?
        for (auto& desc : m_entityDescriptors)
        {
            auto pEntity = aln::New<Entity>();
            desc.InstanciateEntity(pEntity, &typeRegistryService);
            entityMap.m_entities.push_back(pEntity);

            // TODO: what if the map is not loaded yet ?
            entityMap.m_loadingEntities.push_back(pEntity);
        }

        // Resolve spatial relationships
        for (auto& relationship : m_spatialEntitiesRelationships)
        {
            if (relationship.m_parentEntityIndex != InvalidIndex)
            {
                auto pEntity = entityMap.m_entities[relationship.m_entityIndex];
                auto pParentEntity = entityMap.m_entities[relationship.m_parentEntityIndex];

                // TODO: Sockets ?
                pEntity->m_pParentSpatialEntity = pParentEntity;
                pParentEntity->m_attachedEntities.push_back(pEntity);
            }
        }
    }

  public:
    template <class Archive>
    void Serialize(Archive& archive) const
    {
        archive << m_entityDescriptors;
        archive << m_spatialEntitiesRelationships;
    }

    template <class Archive>
    void Deserialize(Archive& archive)
    {
        archive >> m_entityDescriptors;
        archive >> m_spatialEntitiesRelationships;
    }
};

} // namespace aln