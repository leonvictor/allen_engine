#include <iostream>
#include <map>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

template <typename T>
class TypeHelper
{
  public:
    typedef T m_type;
    T CreateType() { return T(); }
};

class ITypeInfo
{
  public:
    // Get the registered type infos. The first call to this method will create the registry.
    static std::unordered_map<std::type_index, std::shared_ptr<ITypeInfo>>& GetRegisteredTypeInfos()
    {
        static std::unordered_map<std::type_index, std::shared_ptr<ITypeInfo>> m_registeredTypeInfos;
        return m_registeredTypeInfos;
    }
};

template <typename T>
class TypeInfo : public ITypeInfo
{
  public:
    // TODO: disable other creation type
    TypeHelper<T> m_typeHelper;
    std::type_index m_ID = std::type_index(typeid(T));
    // TODO: m_ID can be static as TypeInfo<Type> should be a singleton

    static std::shared_ptr<TypeInfo> CreateTypeInfo()
    {
        std::shared_ptr<TypeInfo> pTypeInfo = std::make_shared<TypeInfo>();

        // If typeInfo is already registered, return the registered instance
        if (GetRegisteredTypeInfos().find(pTypeInfo->m_ID) != GetRegisteredTypeInfos().end())
        {
            return static_pointer_cast<TypeInfo>(GetRegisteredTypeInfos()[pTypeInfo->m_ID]);
        }

        GetRegisteredTypeInfos().insert(std::make_pair(pTypeInfo->m_ID, pTypeInfo));
        return pTypeInfo;
    }

    /// @brief Check if a system type is derived from another one
    bool IsDerivedFrom(std::type_index typeIndex)
    {
        auto otherType = GetRegisteredTypeInfos()[typeIndex];
        return true;
    }
};

class Base
{
    // virtual TypeInfo<Base>* GetTypeInfo() = 0;
};

class Derived : public Base
{
  public:
    void DerivedThing() { std::cout << "Derived" << std::endl; }
    // TODO: TypeInfo = static variable.
    static std::shared_ptr<TypeInfo<Derived>> GetTypeInfo()
    {
        return TypeInfo<Derived>::CreateTypeInfo();
    }
};

class DerivedTwice : public Derived
{
  public:
    static std::shared_ptr<TypeInfo<DerivedTwice>> GetTypeInfo()
    {
        return TypeInfo<DerivedTwice>::CreateTypeInfo();
    }
};

template <typename T>
T Truc()
{
    static_assert(std::is_base_of<Base, T>::value);
    T truc = T::GetTypeInfo()->m_typeHelper.CreateType();
    return truc;
}

int main()
{
    DerivedTwice d2 = Truc<DerivedTwice>();
    Derived d = Truc<Derived>();
    // d.DerivedThing();
    bool test = d2.GetTypeInfo()->IsDerivedFrom(d.GetTypeInfo()->m_ID);
    bool test2 = d.GetTypeInfo()->IsDerivedFrom(d2.GetTypeInfo()->m_ID);
    return 0;
}