#pragma once

#include <cctype>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <common/memory.hpp>
#include <common/serialization/binary_archive.hpp>
#include <common/string_id.hpp>

#include "reflected_type.hpp"
#include "type_info.hpp"

namespace aln::reflect
{

// Type descriptors are serializable representation of types instances

class ClassMemberDescriptor
{
    friend class TypeDescriptor;

  private:
    StringID m_typeID;
    std::string m_name;

    std::vector<std::byte> m_value;

  public:
    // Serialization
    template <class Archive>
    void Serialize(Archive& archive) const
    {
        archive << m_typeID;
        archive << m_name;
        archive << m_value;
    }

    template <class Archive>
    void Deserialize(Archive& archive)
    {
        archive >> m_typeID;
        archive >> m_name;
        archive >> m_value;
    }
};

class TypeDescriptor
{
    StringID m_typeID;
    std::vector<ClassMemberDescriptor> m_members;

  public:
    // Serialization
    template <class Archive>
    void Serialize(Archive& archive) const
    {
        archive << m_typeID;
        archive << m_members;
    }

    template <class Archive>
    void Deserialize(Archive& archive)
    {
        archive >> m_typeID;
        archive >> m_members;
    }

    bool IsValid() const { return m_typeID.IsValid(); }

    static void DescribeType(const IReflected* pTypeInstance, TypeDescriptor& typeDesc, const TypeRegistryService* pTypeRegistryService, const reflect::TypeInfo* pTypeInfo)
    {
        for (auto& memberInfo : pTypeInfo->GetMembers())
        {
            auto& memberDescriptor = typeDesc.m_members.emplace_back();
            memberDescriptor.m_typeID = memberInfo.GetTypeID();
            memberDescriptor.m_name = memberInfo.GetName();

            // TODO: the member's type info could be kept in member info
            auto pMemberTypeInfo = pTypeRegistryService->GetTypeInfo(memberInfo.GetTypeID());
            assert(pMemberTypeInfo != nullptr);

            auto pMemberMemory = ((std::byte*) pTypeInstance) + memberInfo.GetOffset();
            if (pMemberTypeInfo->IsPrimitive())
            {
                auto pPrimitiveTypeInfo = dynamic_cast<const PrimitiveTypeInfo*>(pMemberTypeInfo);
                assert(pPrimitiveTypeInfo != nullptr);

                auto archive = BinaryMemoryArchive(memberDescriptor.m_value, IBinaryArchive::IOMode::Write);
                pPrimitiveTypeInfo->Serialize(archive, pMemberMemory);
            }
            else
            {
                // Member is a reflected type, recursively do the same thing
                DescribeType((IReflected*) pMemberMemory, typeDesc, pTypeRegistryService, pMemberTypeInfo);
            }
        }
    }

    /// @brief Describe a type instance by reflecting its value(s)
    virtual void DescribeTypeInstance(const IReflected* pTypeInstance, const TypeRegistryService* pTypeRegistryService, const reflect::TypeInfo* pTypeInfo)
    {
        assert(!IsValid() && pTypeInstance != nullptr);

        // auto pTypeInfo = pTypeInstance->GetTypeInfo();
        m_typeID = pTypeInfo->GetTypeID();
        DescribeType(pTypeInstance, *this, pTypeRegistryService, pTypeInfo);
    }

    /// @brief Instanciate this type with members values set as the original described instance's
    template <typename T>
    T* Instanciate(const TypeRegistryService* pTypeRegistryService)
    {
        assert(pTypeRegistryService != nullptr);
        assert(IsValid());

        auto pTypeInfo = pTypeRegistryService->GetTypeInfo(m_typeID);

        auto memberCount = m_members.size();
        assert(memberCount == pTypeInfo->GetMemberCount());

        auto pInstance = pTypeInfo->CreateTypeInstance<T>();

        for (auto memberIdx = 0; memberIdx < memberCount; ++memberIdx)
        {
            auto pMemberInfo = pTypeInfo->GetMemberInfo(memberIdx);
            auto pMemberTypeInfo = pTypeRegistryService->GetTypeInfo(pMemberInfo->GetTypeID());

            if (pMemberTypeInfo->IsPrimitive())
            {
                auto pMemberPrimitiveTypeInfo = dynamic_cast<const PrimitiveTypeInfo*>(pMemberTypeInfo);

                auto archive = BinaryMemoryArchive(m_members[memberIdx].m_value, IBinaryArchive::IOMode::Read);
                auto pMemberMemory = (std::byte*) pInstance + pMemberInfo->GetOffset();
                pMemberPrimitiveTypeInfo->Deserialize(archive, pMemberMemory);
            }
            else
            {
                // TODO
                assert(false);
            }
        }

        return pInstance;
    }
};

/// @brief Describes a collection of types, usually when they are derived of a common base class
/// Used to instanciate the collection
struct TypeCollectionDescriptor
{
    std::vector<TypeDescriptor> m_descriptors;

    template <class Archive>
    void Serialize(Archive& archive) const
    {
        archive << m_descriptors;
    }

    template <class Archive>
    void Deserialize(Archive& archive)
    {
        archive >> m_descriptors;
        for (auto& descriptor : m_descriptors)
        {
            descriptor.Initialize();
        }
    }

    /// @brief Instanciate the type collection in contiguous memory. Handling the memory is up to the user !
    /// @tparam T Base class of instanciated types
    /// @param outCollection
    template <typename T>
    void InstanciateFixedSizeCollection(std::vector<T*>& outCollection)
    {
        // assert(!outCollection.empty());
        // assert(!m_descriptors.empty());

        // size_t collectionSize = m_descriptors.size();

        // // Calculate the memory requirements for the settings array

        // size_t requiredMemoryAlignment = 0;
        // size_t requiredMemorySize = 0;
        // std::vector<size_t> requiredPaddings;

        // // 1 - First loop to get the maximum required alignement
        // for (auto& descriptor : m_descriptors)
        // {
        //     assert(descriptor.IsInitialized());
        //     requiredMemoryAlignment = std::max(requiredMemoryAlignment, descriptor.m_pTypeInfo->m_alignment);
        // }

        // // 2 - Second one to calculate the total size + paddings
        // /// @todo: How can we merge the first two loops ?
        // requiredPaddings.reserve(collectionSize);
        // for (auto& descriptor : m_descriptors)
        // {
        //     size_t requiredPadding = (requiredMemoryAlignment - (requiredMemorySize % requiredMemoryAlignment)) % requiredMemoryAlignment;
        //     requiredPaddings.push_back(requiredPadding);

        //     requiredMemorySize += descriptor.m_pTypeInfo->m_size + requiredPadding;
        // }

        // // 3 - Allocate the memory for the actual settings data
        // // /!\ The memory is not handled by the class but by the loader !
        // auto pMemory = (std::byte*) aln::Allocate(requiredMemorySize, requiredMemoryAlignment);

        // // 4 - Placement new each type in contiguous memory
        // // and add a ptr to the definition's collection
        // outCollection.reserve(collectionSize);
        // for (auto i = 0; i < collectionSize; ++i)
        // {
        //     pMemory += requiredPaddings[i];
        //     // outCollection.push_back(m_descriptors[i].CreateTypeInPlace<T>(pMemory));
        //     // pMemory += m_descriptors[i].m_pTypeInfo->m_size;
        // }
    }

    template <typename T>
    void DeleteFixedSizeCollection(std::vector<T*>& collection)
    {
        for (auto& pInstance : collection)
        {
            pInstance->~T();
        }
        aln::Free(collection[0]);
    }

    void InstanciateDynamicCollection()
    {
        // TODO
    }
};
} // namespace aln::reflect