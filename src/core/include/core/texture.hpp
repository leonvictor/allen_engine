#pragma once

#include <assets/asset.hpp>
#include <graphics/resources/image.hpp>

namespace aln
{
/// @brief Temporary asset for textures
class Texture : public IAsset
{
    ALN_REGISTER_ASSET_TYPE("text");

    // TODO: friends shouldn't be necessary
    friend class TextureLoader;
    friend class MeshComponent;

  private:
    vkg::resources::Image m_image;

  public:
    inline const vk::DescriptorImageInfo GetDescriptor() const { return m_image.GetDescriptor(); }
};

ALN_REGISTER_PRIMITIVE(Texture);
} // namespace aln