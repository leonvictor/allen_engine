/// Descriptors for types from the standard library.

#include "reflection.hpp"

#include <string>

namespace aln::reflect
{

/// @brief A type descriptor for std::string
struct TypeDescriptor_StdString : TypeDescriptor
{
    TypeDescriptor_StdString() : TypeDescriptor(aln::StringID("std::string")) {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<std::string>()
{
    static TypeDescriptor_StdString typeDesc;
    return &typeDesc;
}

/// @brief Type descriptors for std::vector
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
};

// Partially specialize TypeResolver<> for std::vectors:
template <typename T>
struct TypeResolver<std::vector<T>>
{
  public:
    static TypeDescriptor* get()
    {
        static TypeDescriptor_StdVector typeDesc{(T*) nullptr};
        return &typeDesc;
    }
};
} // namespace aln::reflect