#include <assert.h>
#include <iostream>
#include <map>
#include <set>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

class ITypeHelper
{
  public:
    virtual std::shared_ptr<void> CreateType() = 0;
    virtual bool IsBaseOf(std::shared_ptr<void> pType) = 0;
};

template <typename TBase, typename T>
class TypeHelper : public ITypeHelper
{
    static_assert(std::is_base_of_v<TBase, T>);

  public:
    std::shared_ptr<void> CreateType() override
    {
        std::shared_ptr<void> type = std::make_shared<T>();
        return type;
    }

    bool IsBaseOf(std::shared_ptr<void> pType)
    {
        return dynamic_pointer_cast<T>(static_pointer_cast<TBase>(pType)) != nullptr;
    }
};

template <typename TBase>
class TypeInfo
{
  public:
    // TODO: disable other creation type
    std::type_index m_ID = std::type_index(typeid(TBase));
    std::shared_ptr<ITypeHelper> m_pTypeHelper;
    std::set<std::type_index> m_bases;

    /// @brief Get the registered type infos. The first call to this method will create the registry.
    static std::unordered_map<std::type_index, std::shared_ptr<TypeInfo>>&
    GetRegisteredTypeInfos()
    {
        static std::unordered_map<std::type_index, std::shared_ptr<TypeInfo>> registeredTypeInfos;
        return registeredTypeInfos;
    }

    template <typename T>
    static std::shared_ptr<TypeInfo> CreateTypeInfo()
    {
        auto id = std::type_index(typeid(T));

        // If typeInfo is already registered, return the registered instance
        if (GetRegisteredTypeInfos().find(id) != GetRegisteredTypeInfos().end())
        {
            return static_pointer_cast<TypeInfo>(GetRegisteredTypeInfos()[id]);
        }

        std::shared_ptr<TypeInfo> pTypeInfo = std::make_shared<TypeInfo>();
        pTypeInfo->m_ID = id;
        pTypeInfo->m_pTypeHelper = std::make_shared<TypeHelper<TBase, T>>();

        // Loop over registered types and add inheritance links.
        // TODO: This is quite wonky as it requires creating an instance of each
        auto pTypeInstance = pTypeInfo->m_pTypeHelper->CreateType();
        for (auto it : GetRegisteredTypeInfos())
        {
            auto otherInstance = it.second->m_pTypeHelper->CreateType();
            if (dynamic_pointer_cast<T>(static_pointer_cast<TBase>(otherInstance)) != nullptr)
            {
                // This TypeInfo is base of otherInstance
                it.second->m_bases.insert(id);
            }
            if (it.second->m_pTypeHelper->IsBaseOf(pTypeInstance))
            {
                // it.second is base of this TypeInfo
                pTypeInfo->m_bases.insert(it.second->m_ID);
            }
        }

        GetRegisteredTypeInfos().insert(std::make_pair(id, pTypeInfo));
        return pTypeInfo;
    }

    /// @brief Check if a system type is derived from another one
    bool IsDerivedFrom(std::type_index typeIndex)
    {
        return m_bases.find(typeIndex) != m_bases.end();
    }
};

class Base
{
  public:
    virtual ~Base() {}
    virtual std::shared_ptr<TypeInfo<Base>> GetTypeInfo() = 0;
};

class Derived : public Base
{
  public:
    // TODO: TypeInfo = static variable.
    static std::shared_ptr<TypeInfo<Base>> GetTypeInfo()
    {
        return TypeInfo<Base>::CreateTypeInfo<Derived>();
    }
};

class DerivedTwice : public Derived
{
  public:
    static std::shared_ptr<TypeInfo<Base>> GetTypeInfo()
    {
        return TypeInfo<Base>::CreateTypeInfo<DerivedTwice>();
    }
};

template <typename T>
std::shared_ptr<T> Truc()
{
    static_assert(std::is_base_of_v<Base, T>);
    std::shared_ptr<T> truc = std::static_pointer_cast<T>(T::GetTypeInfo()->m_pTypeHelper->CreateType());
    return truc;
}

int main()
{
    std::shared_ptr<DerivedTwice> d2 = Truc<DerivedTwice>();
    std::shared_ptr<Derived> d = Truc<Derived>();

    bool test = d2->GetTypeInfo()->IsDerivedFrom(d->GetTypeInfo()->m_ID);
    assert(test);
    bool test2 = d->GetTypeInfo()->IsDerivedFrom(d2->GetTypeInfo()->m_ID);
    assert(!test);
    return 0;
}