/// Descriptors for asset types.

#include <material.hpp>
#include <mesh.hpp>
#include <texture.hpp>

#include <anim/animation_clip.hpp>

#include <reflection/reflection.hpp>

namespace aln::reflect
{

//--------------------------------------------------------
// Type descriptors for Meshes
//--------------------------------------------------------
struct TypeDescriptor_Mesh : TypeDescriptor
{
    TypeDescriptor_Mesh() : TypeDescriptor{std::type_index(typeid(Mesh))} {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<Mesh>()
{
    static TypeDescriptor_Mesh typeDesc;
    return &typeDesc;
}

//--------------------------------------------------------
// Type descriptors for Textures
//--------------------------------------------------------
struct TypeDescriptor_Texture : TypeDescriptor
{
    TypeDescriptor_Texture() : TypeDescriptor{std::type_index(typeid(Texture))} {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<Texture>()
{
    static TypeDescriptor_Texture typeDesc;
    return &typeDesc;
}

//--------------------------------------------------------
// Type descriptors for Materials
//--------------------------------------------------------
struct TypeDescriptor_Material : TypeDescriptor
{
    TypeDescriptor_Material() : TypeDescriptor{std::type_index(typeid(Material))} {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<Material>()
{
    static TypeDescriptor_Material typeDesc;
    return &typeDesc;
}

/// @brief Type descriptor for animation clips
struct TypeDescriptor_AnimationClip : TypeDescriptor
{
    TypeDescriptor_AnimationClip() : TypeDescriptor{std::type_index(typeid(AnimationClip))} {}
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<AnimationClip>()
{
    static TypeDescriptor_AnimationClip typeDesc;
    return &typeDesc;
}

} // namespace aln::reflect