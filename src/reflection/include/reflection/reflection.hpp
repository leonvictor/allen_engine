#pragma once

#include <cctype>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <typeinfo>
#include <vector>

#include <imgui.h>

#include <common/memory.hpp>
#include <iostream>

namespace aln::reflect
{

// ImGui method to set the context and allocator functions in case reflection is in a separate library.
void SetImGuiContext(ImGuiContext* pContext);
void SetImGuiAllocatorFunctions(ImGuiMemAllocFunc* pAllocFunc, ImGuiMemFreeFunc* pFreeFunc, void** pUserData);

/// @brief A type that has been manually marked for reflection
template <typename T>
concept Reflected = requires(T a) {
                        T::Reflection;
                    };

/// @brief Base class of all type descriptors
struct TypeDescriptor
{
    const char* name;
    size_t size;
    std::type_index m_typeIndex;

    std::function<void*()> m_createType;

    TypeDescriptor(const char* name, size_t size, std::type_index typeIndex);
    virtual ~TypeDescriptor() {}

    template <typename T>
    T* CreateType() const { return (T*) m_createType(); }

    /// @brief Return the full type name.
    virtual std::string GetFullName() const { return name; }

    /// @brief Return a prettified version of the type's name.
    /// @todo: Cache the prettified name
    virtual std::string GetPrettyName() const
    {
        // Use std::format (C++20). Not available in most compilers as of 04/06/2021
        std::string prettyName = std::string(name);

        // Remove the namespace info
        prettyName = prettyName.substr(prettyName.rfind(":") + 1);
        return prettyName;
    }

    /// @brief Print a text representation of an object of the reflected type.
    virtual void Dump(const void* obj, int indentLevel = 0) const = 0;

    friend bool operator==(const TypeDescriptor& a, const TypeDescriptor& b);
    friend bool operator!=(const TypeDescriptor& a, const TypeDescriptor& b);
};

std::map<std::type_index, TypeDescriptor*>& RegisteredTypes();
void RegisterType(std::type_index typeIndex, TypeDescriptor* pTypeDescriptor);
const TypeDescriptor* GetType(const std::type_index& typeIndex);

/// @brief Retrieve a list of all the types registered to a specific scope.
/// @param: scopeName: The scope to retrieve from.
std::vector<TypeDescriptor*>& GetTypesInScope(const std::string& scopeName);

//--------------------------------------------------------
// Finding type descriptors
//--------------------------------------------------------

// Declare the function template that handles primitive types such as int, std::string, etc.:
template <typename T>
TypeDescriptor* GetPrimitiveDescriptor();

/// @brief A helper class to find TypeDescriptors in different ways.
struct DefaultResolver
{
    // This version is called if T has a static member named "Reflection":
    template <Reflected T>
    static TypeDescriptor* get()
    {
        return &T::Reflection;
    }

    // This version is called otherwise:
    template <typename T>
    static TypeDescriptor* get()
    {
        return GetPrimitiveDescriptor<T>();
    }
};

/// @brief This is the primary class template for finding all TypeDescriptors:
/// @note: https://www.youtube.com/watch?v=aZNhSOIvv1Q
// "C++Now 2019: JeanHeyd Meneide “The Plan for Tomorrow: Extension Points in C++ Applications”"
template <typename T, typename C = void>
struct TypeResolver
{
    static TypeDescriptor* get()
    {
        return DefaultResolver::get<T>();
    }
};

/// @brief Type descriptors for user-defined structs/classes
struct TypeDescriptor_Struct : TypeDescriptor
{
    struct Member
    {
        const char* name;
        const char* displayName;
        size_t offset;
        TypeDescriptor* type;

        std::string GetPrettyName() const;
    };

    std::vector<Member> members;

    TypeDescriptor_Struct(void (*init)(TypeDescriptor_Struct*), std::type_index typeIndex)
        : TypeDescriptor(nullptr, 0, typeIndex)
    {
        init(this);
    }

    TypeDescriptor_Struct(const char* name, size_t size, const std::initializer_list<Member>& init)
        : TypeDescriptor(nullptr, 0, std::type_index(typeid(nullptr))), members(init) {}

    virtual void Dump(const void* obj, int indentLevel) const override;
};

/// @brief Register the current type for class reflection.
#define ALN_REGISTER_TYPE()                                             \
    friend struct aln::reflect::DefaultResolver;                        \
                                                                        \
  public:                                                               \
    static const aln::reflect::TypeDescriptor_Struct* GetStaticType();  \
    virtual const aln::reflect::TypeDescriptor_Struct* GetType() const; \
                                                                        \
  private:                                                              \
    static aln::reflect::TypeDescriptor_Struct Reflection;              \
    static void InitReflection(aln::reflect::TypeDescriptor_Struct*);

#define ALN_REGISTER_IMPL_BEGIN(scope, type)                                                                   \
    aln::reflect::TypeDescriptor_Struct type::Reflection(type::InitReflection, std::type_index(typeid(type))); \
    const aln::reflect::TypeDescriptor_Struct* type::GetType() const                                           \
    {                                                                                                          \
        return &Reflection;                                                                                    \
    }                                                                                                          \
                                                                                                               \
    const aln::reflect::TypeDescriptor_Struct* type::GetStaticType()                                           \
    {                                                                                                          \
        return &Reflection;                                                                                    \
    }                                                                                                          \
                                                                                                               \
    void type::InitReflection(aln::reflect::TypeDescriptor_Struct* typeDesc)                                   \
    {                                                                                                          \
        auto& scopedTypes = aln::reflect::GetTypesInScope(#scope);                                             \
                                                                                                               \
        using T = type;                                                                                        \
        typeDesc->name = #type;                                                                                \
        typeDesc->size = sizeof(T);                                                                            \
        typeDesc->m_typeIndex = std::type_index(typeid(T));                                                    \
        typeDesc->members = {

#define ALN_REFLECT_MEMBER(name, displayName) \
    {#name, #displayName, offsetof(T, name), aln::reflect::TypeResolver<decltype(T::name)>::get()},

#define ALN_REGISTER_IMPL_END()                              \
    }                                                        \
    ;                                                        \
    typeDesc->m_createType = []() { return aln::New<T>(); }; \
    scopedTypes.push_back(typeDesc);                         \
    } //

/// @brief Abstract types are not actually reflected, but they can be marked to declare the reflection system's functions as virtual.
// This allows us to dynamically get the type descriptor of a derived type
#define ALN_REGISTER_ABSTRACT_TYPE()                                        \
  public:                                                                   \
    virtual const aln::reflect::TypeDescriptor_Struct* GetType() const = 0; \
                                                                            \
  private:

} // namespace aln::reflect