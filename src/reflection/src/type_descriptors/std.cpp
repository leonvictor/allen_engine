/// Descriptors for types from the standard library.

#include "reflection.hpp"

#include <string>

namespace aln::reflect
{

/// @brief A type descriptor for std::string
struct TypeDescriptor_StdString : TypeDescriptor
{
    TypeDescriptor_StdString() : TypeDescriptor{"std::string", sizeof(std::string), std::type_index(typeid(std::string))} {}
    virtual void Dump(const void* obj, int) const override
    {
        std::cout << "std::string{\"" << *(const std::string*) obj << "\"}";
    }
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