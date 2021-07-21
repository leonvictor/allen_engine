#pragma once

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include <imgui.h>

namespace aln::reflect
{

void SetImGuiContext(ImGuiContext* pContext);
void SetImGuiAllocatorFunctions(ImGuiMemAllocFunc* pAllocFunc, ImGuiMemFreeFunc* pFreeFunc, void** pUserData);

//--------------------------------------------------------
// Base class of all type descriptors
//--------------------------------------------------------

struct TypeDescriptor
{
    const char* name;
    size_t size;

    TypeDescriptor(const char* name, size_t size) : name{name}, size{size} {}
    virtual ~TypeDescriptor() {}
    virtual std::string GetFullName() const { return name; }
    virtual void Dump(const void* obj, int indentLevel = 0) const = 0;
    virtual void InEditor(void* obj, const char* fieldName = "") const = 0;
};

//--------------------------------------------------------
// Finding type descriptors
//--------------------------------------------------------

// Declare the function template that handles primitive types such as int, std::string, etc.:
template <typename T>
TypeDescriptor* GetPrimitiveDescriptor();

// A helper class to find TypeDescriptors in different ways:
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

    virtual void Dump(const void* obj, int indentLevel) const override
    {
        std::cout << name << " {" << std::endl;
        for (const Member& member : members)
        {
            std::cout << std::string(4 * (indentLevel + 1), ' ') << member.name << " = ";
            member.type->Dump((char*) obj + member.offset, indentLevel + 1);
            std::cout << std::endl;
        }
        std::cout << std::string(4 * indentLevel, ' ') << "}";
    }

    virtual void InEditor(void* obj, const char* fieldName = "") const override;
};

/// @brief Register the currect type for class reflection.
#define ALN_REGISTER_TYPE()                                           \
    friend struct aln::reflect::DefaultResolver;                      \
    static aln::reflect::TypeDescriptor_Struct Reflection;            \
    static void InitReflection(aln::reflect::TypeDescriptor_Struct*); \
    virtual const aln::reflect::TypeDescriptor_Struct* GetStaticType(); // virtual so we can specialize

#define ALN_REGISTER_IMPL_BEGIN(type)                                           \
    aln::reflect::TypeDescriptor_Struct type::Reflection{type::InitReflection}; \
    const aln::reflect::TypeDescriptor_Struct* type::GetStaticType()            \
    {                                                                           \
        return &Reflection;                                                     \
    }                                                                           \
                                                                                \
    void type::InitReflection(aln::reflect::TypeDescriptor_Struct* typeDesc)    \
    {                                                                           \
        using T = type;                                                         \
        typeDesc->name = #type;                                                 \
        typeDesc->size = sizeof(T);                                             \
        typeDesc->members = {

#define ALN_REFLECT_MEMBER(name) \
    {#name, offsetof(T, name), aln::reflect::TypeResolver<decltype(T::name)>::get()},

#define ALN_REGISTER_IMPL_END() \
    }                           \
    ;                           \
    }

//--------------------------------------------------------
// Type descriptors for std::vector
//--------------------------------------------------------

struct TypeDescriptor_StdVector : TypeDescriptor
{
    TypeDescriptor* itemType;
    size_t (*getSize)(const void*);
    const void* (*getItem)(const void*, size_t);

    template <typename ItemType>
    TypeDescriptor_StdVector(ItemType*)
        : TypeDescriptor{"std::vector<>", sizeof(std::vector<ItemType>)},
          itemType{TypeResolver<ItemType>::get()}
    {
        getSize = [](const void* vecPtr) -> size_t
        {
            const std::vector<ItemType>& vec = *(const std::vector<ItemType>*) vecPtr;
            return vec.size();
        };
        getItem = [](const void* vecPtr, size_t index) -> const void*
        {
            const std::vector<ItemType>& vec = *(const std::vector<ItemType>*) vecPtr;
            return &vec[index];
        };
    }

    virtual std::string GetFullName() const override
    {
        return std::string("std::vector<") + itemType->GetFullName() + ">";
    }

    virtual void Dump(const void* obj, int indentLevel) const override
    {
        size_t numItems = getSize(obj);
        std::cout << GetFullName();
        if (numItems == 0)
        {
            std::cout << "{}";
        }
        else
        {
            std::cout << "{" << std::endl;
            for (size_t index = 0; index < numItems; index++)
            {
                std::cout << std::string(4 * (indentLevel + 1), ' ') << "[" << index << "] ";
                itemType->Dump(getItem(obj, index), indentLevel + 1);
                std::cout << std::endl;
            }
            std::cout << std::string(4 * indentLevel, ' ') << "}";
        }
    }
};

// Partially specialize TypeResolver<> for std::vectors:
template <typename T>
class TypeResolver<std::vector<T>>
{
  public:
    static TypeDescriptor* get()
    {
        static TypeDescriptor_StdVector typeDesc{(T*) nullptr};
        return &typeDesc;
    }
};

} // namespace aln::reflect