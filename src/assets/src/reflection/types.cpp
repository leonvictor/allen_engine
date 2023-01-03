/// Descriptors for custom color types.
#include <reflection/reflection.hpp>

#include "asset_id.hpp"

#include <string>

namespace aln::reflect
{

/// @brief A type descriptor for RGBAColor
struct TypeDescriptor_AssetID : TypeDescriptor
{
    TypeDescriptor_AssetID() : TypeDescriptor{std::type_index(typeid(aln::AssetID))} {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<aln::AssetID>()
{
    static TypeDescriptor_AssetID typeDesc;
    return &typeDesc;
}
} // namespace aln::reflect
