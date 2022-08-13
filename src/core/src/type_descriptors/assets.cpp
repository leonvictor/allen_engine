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
    TypeDescriptor_Mesh() : TypeDescriptor{"Mesh", sizeof(Mesh), std::type_index(typeid(Mesh))} {}
    virtual void Dump(const void* obj, int) const override
    {
        // std::cout << "std::string{\"" << *(const std::string*) obj << "\"}";
    }
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
    TypeDescriptor_Texture() : TypeDescriptor{"Texture", sizeof(Texture), std::type_index(typeid(Texture))} {}
    virtual void Dump(const void* obj, int) const override
    {
        // std::cout << "std::string{\"" << *(const std::string*) obj << "\"}";
    }
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
    TypeDescriptor_Material() : TypeDescriptor{"Material", sizeof(Material), std::type_index(typeid(Material))} {}
    virtual void Dump(const void* obj, int) const override {}
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
    TypeDescriptor_AnimationClip() : TypeDescriptor{"AnimationClip", sizeof(AnimationClip), std::type_index(typeid(AnimationClip))} {}
    virtual void Dump(const void* obj, int) const override
    {
        // std::cout << "std::string{\"" << *(const std::string*) obj << "\"}";
    }
};

template <>
TypeDescriptor* GetPrimitiveDescriptor<AnimationClip>()
{
    static TypeDescriptor_AnimationClip typeDesc;
    return &typeDesc;
}

} // namespace aln::reflect