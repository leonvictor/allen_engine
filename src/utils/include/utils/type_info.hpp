#pragma once

#include <map>
#include <memory>
#include <set>
#include <typeindex>
#include <unordered_map>

namespace aln::utils
{
class ITypeHelper
{
  public:
    /// @brief Factory method to create an object of the type described by this TypeHelper.
    virtual std::shared_ptr<void> CreateType() const = 0;

    /// @brief Whether the type of the passed pointer is
    virtual bool IsBaseOf(const std::shared_ptr<void> pOtherType) const = 0;
};

/// @brief Holds common routines related to the custom type reflection system (Creation, inheritance tests)
/// @param TBase: The shared base type used by the parent TypeInfo
/// @param T: The specialized class this TypeHelper represents.
template <typename TBase, typename T>
class TypeHelper : public ITypeHelper
{
    static_assert(std::is_base_of_v<TBase, T>);

  public:
    inline std::shared_ptr<void> CreateType() const override
    {
        std::shared_ptr<void> pType = std::make_shared<T>();
        return pType;
    }

    inline bool IsBaseOf(const std::shared_ptr<void> pOtherType) const
    {
        // TODO: Accept std::shared_ptr<TBase>
        // Difficult for now as the virtual call doesn't know about TBase
        auto pBaseType = std::static_pointer_cast<TBase>(pOtherType);
        return std::dynamic_pointer_cast<T>(pBaseType) != nullptr;
    }
};

/// @brief Simple custom type reflection. Manages types inheriting from the abstract base class TBase.
template <typename TBase>
class TypeInfo
{
  private:
    std::set<std::type_index> m_baseTypes;
    std::set<std::type_index> m_derivedTypes;

  public:
    // TODO: disable other creation methods
    std::type_index m_ID = std::type_index(typeid(TBase));
    std::shared_ptr<ITypeHelper> m_pTypeHelper;

    /// @brief Get the registered type infos. The first call to this method will create the registry.
    static std::unordered_map<std::type_index, std::shared_ptr<TypeInfo>>& GetRegisteredTypeInfos()
    {
        static std::unordered_map<std::type_index, std::shared_ptr<TypeInfo>> registeredTypeInfos;
        return registeredTypeInfos;
    }

    /// @brief Get the type info associated to T. The first call to this method will construct and register the TypeInfo.
    template <typename T>
    static std::shared_ptr<TypeInfo> GetTypeInfo()
    {
        auto id = std::type_index(typeid(T));

        // If the requested TypeInfo is already registered, return the registered instance
        if (GetRegisteredTypeInfos().find(id) != GetRegisteredTypeInfos().end())
        {
            return std::static_pointer_cast<TypeInfo>(GetRegisteredTypeInfos()[id]);
        }

        // Otherwise build the TypeInfo
        std::shared_ptr<TypeInfo> pTypeInfo = std::make_shared<TypeInfo>();
        pTypeInfo->m_ID = id;
        pTypeInfo->m_pTypeHelper = std::make_shared<TypeHelper<TBase, T>>();

        // Loop over registered types and add inheritance links.
        // TODO: This is quite wonky as it requires creating an instance of each type.
        auto pTypeInstance = pTypeInfo->m_pTypeHelper->CreateType();
        for (auto it : GetRegisteredTypeInfos())
        {
            auto otherInstance = it.second->m_pTypeHelper->CreateType();
            if (pTypeInfo->m_pTypeHelper->IsBaseOf(otherInstance))
            {
                // This TypeInfo is base of otherInstance
                it.second->m_baseTypes.insert(id);
                pTypeInfo->m_derivedTypes.insert(id);
            }
            if (it.second->m_pTypeHelper->IsBaseOf(pTypeInstance))
            {
                // otherInstance is base of this TypeInfo
                pTypeInfo->m_baseTypes.insert(it.second->m_ID);
                it.second->m_derivedTypes.insert(id);
            }
        }

        // Register the TypeInfo
        GetRegisteredTypeInfos().insert(std::make_pair(id, pTypeInfo));
        return pTypeInfo;
    }

    /// @brief Check if a type is derived from another one
    inline bool IsDerivedFrom(const std::type_index& typeIndex) const
    {
        return m_baseTypes.find(typeIndex) != m_baseTypes.end();
    }

    /// @brief Check if a type is the base of another one
    inline bool IsBaseOf(const std::type_index& typeIndex) const
    {
        return m_derivedTypes.find(typeIndex) != m_derivedTypes.end();
    }

    const bool operator==(const TypeInfo& other) const { return m_ID == other.m_ID; }
    const bool operator!=(const TypeInfo& other) const { return !operator==(other); }
};
} // namespace aln::utils