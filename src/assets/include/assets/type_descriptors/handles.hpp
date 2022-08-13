#pragma once

#include "../asset.hpp"
#include "../handle.hpp"

#include <reflection/reflection.hpp>

namespace aln::reflect
{

//--------------------------------------------------------
// A type descriptor for asset handles
//--------------------------------------------------------
struct TypeDescriptor_AssetHandle : TypeDescriptor
{
    TypeDescriptor* assetType;
    void* (*getAsset)(void*);

    template <AssetType T>
    TypeDescriptor_AssetHandle(T*) : TypeDescriptor{"AssetHandle", sizeof(AssetHandle<T>), std::type_index(typeid(AssetHandle<T>))},
                                     assetType{TypeResolver<T>::get()}
    {
        getAsset = [](void* ptr) -> void* {
            return ((AssetHandle<T>*) ptr)->get();
        };
    }

    virtual void Dump(const void* obj, int) const override;
};

template <AssetType T>
struct TypeResolver<AssetHandle<T>>
{
    static TypeDescriptor* get()
    {
        static TypeDescriptor_AssetHandle typeDesc{(T*) nullptr};
        return &typeDesc;
    }
};
} // namespace aln::reflect