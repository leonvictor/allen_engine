#pragma once

#include <common/types.hpp>
#include <common/string_id.hpp>
#include <reflection/type_descriptor.hpp>

#include <string>
#include <vector>

namespace aln
{

class Entity;
class IComponent;
class EntityMap;
class LoadingContext;

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

    static uint32_t GetComponentIndex(const Entity* pEntity, const IComponent* pComponent);

  private:
    // TODO: Unique identifier, but UUID is not garanteed to stay the same between executions.
    // Use name ? As a StringID maybe ?
    std::string m_name;
    std::vector<ComponentDescriptor> m_componentDescriptors;
    std::vector<SystemDescriptor> m_systemDescriptors;
    std::vector<SpatialComponentsRelationship> m_spatialComponentsRelationships;

  public:
    EntityDescriptor() = default;
    EntityDescriptor(const Entity* pEntity, const TypeRegistryService* pTypeRegistryService);

    void InstanciateEntity(Entity* pEntity, const TypeRegistryService* pTypeRegistryService);

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

/// @brief Represents a collection of entities, and can be used to instanciate them all in contiguous memory
class EntityMapDescriptor
{
  private:
    struct SpatialEntitiesRelationship
    {
        uint32_t m_entityIndex = InvalidIndex;
        uint32_t m_parentEntityIndex = InvalidIndex;
    };

    static uint32_t GetEntityIndex(const EntityMap& entityMap, const Entity* pEntity);

  private:
    std::vector<EntityDescriptor> m_entityDescriptors;
    std::vector<SpatialEntitiesRelationship> m_spatialEntitiesRelationships;

  public:
    EntityMapDescriptor() = default;
    EntityMapDescriptor(const EntityMap& entityMap, const TypeRegistryService& typeRegistryService);

    void InstanciateEntityMap(EntityMap& entityMap, const LoadingContext& loadingContext, const TypeRegistryService& typeRegistryService);

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