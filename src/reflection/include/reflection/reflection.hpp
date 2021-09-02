#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <imgui.h>

namespace aln::reflect
{

// ImGui method to set the context and allocator functions in case reflection is in a separate library.
void SetImGuiContext(ImGuiContext* pContext);
void SetImGuiAllocatorFunctions(ImGuiMemAllocFunc* pAllocFunc, ImGuiMemFreeFunc* pFreeFunc, void** pUserData);

class ITypeHelper
{
  public:
    virtual void* CreateType() = 0;
};

struct TypeHelperResolver
{
    // This version is called if T is constructible:
    template <typename T,
        typename std::enable_if<std::is_constructible<T>::value, bool>::type = true>
    static T* CreateType()
    {
        return new T();
    }

    // This version is called otherwise:
    template <typename T,
        typename std::enable_if<!std::is_constructible<T>::value, bool>::type = true>
    static T* CreateType()
    {
        throw std::runtime_error("Failed creating from type: " << typeid(T).name() << " is not constructible.");
    }
};

template <typename T>
struct TypeHelper : public ITypeHelper
{
    void* CreateType() override { return TypeHelperResolver::CreateType<T>(); }
};
//--------------------------------------------------------
// Base class of all type descriptors
//--------------------------------------------------------

struct TypeDescriptor
{
    const char* name;
    size_t size;

    std::shared_ptr<ITypeHelper> typeHelper;

    TypeDescriptor(const char* name, size_t size) : name{name}, size{size} {}
    virtual ~TypeDescriptor() {}

    /// @brief Return the full type name.
    virtual std::string GetFullName() const { return name; }

    /// @brief Return a prettified version of the type's name.
    virtual std::string GetPrettyName() const
    {
        // Use std::format (C++20). Not available in most compilers as of 04/06/2021
        std::string prettyName = std::string(name);
        // Remove the namespace info
        prettyName = prettyName.substr(prettyName.rfind(":") + 1);
        // TODO: Remove member variable prefix m_ if necessary
        return prettyName;
    }

    /// @brief Print a text representation of an object of the reflected type.
    virtual void Dump(const void* obj, int indentLevel = 0) const = 0;

    /// @brief Display an object of this reflected type in the editor.
    /// Override to define custom behavior.
    /// @param obj: The object to display
    /// @param fieldName: Display name of the object
    virtual void InEditor(void* obj, const char* fieldName = "") const = 0;
};

/// @brief Retrieve a list of all the types registered to a specific scope.
/// @param: scopeName: The scope to retrieve from.
std::vector<TypeDescriptor*>& GetTypesInScope(std::string scopeName);

//--------------------------------------------------------
// Finding type descriptors
//--------------------------------------------------------

// Declare the function template that handles primitive types such as int, std::string, etc.:
template <typename T>
TypeDescriptor* GetPrimitiveDescriptor();

/// @brief A helper class to find TypeDescriptors in different ways.
struct DefaultResolver
{
    template <typename T>
    static char func(decltype(&T::Reflection));
    template <typename T>
    static int func(...);
    template <typename T>
    struct IsReflected
    {
        enum
        {
            value = (sizeof(func<T>(nullptr)) == sizeof(char))
        };
    };

    // This version is called if T has a static member named "Reflection":
    template <typename T, typename std::enable_if<IsReflected<T>::value, int>::type = 0>
    static TypeDescriptor* get()
    {
        return &T::Reflection;
    }

    // This version is called otherwise:
    template <typename T, typename std::enable_if<!IsReflected<T>::value, int>::type = 0>
    static TypeDescriptor* get()
    {
        return GetPrimitiveDescriptor<T>();
    }
};

// This is the primary class template for finding all TypeDescriptors:
template <typename T>
struct TypeResolver
{
    static TypeDescriptor* get()
    {
        return DefaultResolver::get<T>();
    }
};

//--------------------------------------------------------
// Type descriptors for user-defined structs/classes
//--------------------------------------------------------

struct TypeDescriptor_Struct : TypeDescriptor
{
    struct Member
    {
        const char* name;
        size_t offset;
        TypeDescriptor* type;
    };

    std::vector<Member> members;

    TypeDescriptor_Struct(void (*init)(TypeDescriptor_Struct*))
        : TypeDescriptor(nullptr, 0)
    {
        init(this);
    }

    TypeDescriptor_Struct(const char* name, size_t size, const std::initializer_list<Member>& init)
        : TypeDescriptor(nullptr, 0), members(init)
    {
    }

    virtual void Dump(const void* obj, int indentLevel) const override;
    virtual void InEditor(void* obj, const char* fieldName = "") const override;
};

/// @brief Register the currect type for class reflection.
#define ALN_REGISTER_TYPE()                                           \
    friend struct aln::reflect::DefaultResolver;                      \
    static aln::reflect::TypeDescriptor_Struct Reflection;            \
    static void InitReflection(aln::reflect::TypeDescriptor_Struct*); \
    virtual const aln::reflect::TypeDescriptor_Struct* GetStaticType(); // virtual so we can specialize

#define ALN_REGISTER_IMPL_BEGIN(scope, type)                                    \
    aln::reflect::TypeDescriptor_Struct type::Reflection{type::InitReflection}; \
    const aln::reflect::TypeDescriptor_Struct* type::GetStaticType()            \
    {                                                                           \
        return &Reflection;                                                     \
    }                                                                           \
    void type::InitReflection(aln::reflect::TypeDescriptor_Struct* typeDesc)    \
    {                                                                           \
        auto& scopedTypes = aln::reflect::GetTypesInScope(#scope);              \
                                                                                \
        using T = type;                                                         \
        typeDesc->typeHelper = std::make_shared<aln::reflect::TypeHelper<T>>(); \
        typeDesc->name = #type;                                                 \
        typeDesc->size = sizeof(T);                                             \
        typeDesc->members = {

#define ALN_REFLECT_MEMBER(name) \
    {#name, offsetof(T, name), aln::reflect::TypeResolver<decltype(T::name)>::get()},

#define ALN_REGISTER_IMPL_END()      \
    }                                \
    ;                                \
    scopedTypes.push_back(typeDesc); \
    }

} // namespace aln::reflect