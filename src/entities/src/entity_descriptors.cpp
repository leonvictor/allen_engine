#include "entity_descriptors.hpp"

#include <reflection/type_descriptor.hpp>

#include "component.hpp"
#include "entity.hpp"
#include "entity_map.hpp"
#include "spatial_component.hpp"

namespace aln
{
uint32_t EntityDescriptor::GetComponentIndex(const Entity* pEntity, const IComponent* pComponent)
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

EntityDescriptor::EntityDescriptor(const Entity* pEntity, const TypeRegistryService* pTypeRegistryService)
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

void EntityDescriptor::InstanciateEntity(Entity* pEntity, const TypeRegistryService* pTypeRegistryService)
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

uint32_t EntityMapDescriptor::GetEntityIndex(const EntityMap& entityMap, const Entity* pEntity)
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

EntityMapDescriptor::EntityMapDescriptor(const EntityMap& entityMap, const TypeRegistryService& typeRegistryService)
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

void EntityMapDescriptor::InstanciateEntityMap(EntityMap& entityMap, const LoadingContext& loadingContext, const TypeRegistryService& typeRegistryService)
{
    auto entityCount = m_entityDescriptors.size();
    entityMap.m_entities.reserve(entityCount);
    entityMap.m_loadingEntities.reserve(entityCount);
    entityMap.m_entityLookupMap.reserve(entityCount);

    // TODO: Parallelize ?
    for (auto& desc : m_entityDescriptors)
    {
        auto pEntity = aln::New<Entity>();
        desc.InstanciateEntity(pEntity, &typeRegistryService);

        entityMap.m_entities.push_back(pEntity);
        entityMap.m_entityLookupMap[pEntity->GetID()] = pEntity;

        // TODO: what if the map is not loaded yet ?
        pEntity->LoadComponents(loadingContext);
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
} // namespace aln