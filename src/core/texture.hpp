#pragma once

#include <assets/asset.hpp>
#include <graphics/resources/image.hpp>

namespace aln
{
/// @brief Temporary asset for textures
class Texture : public IAsset
{
    // TODO: friends shouldn't be necessary
    friend class TextureLoader;
    friend class MeshRenderer;

  private:
    vkg::resources::Image m_image;

  public:
    Texture(AssetGUID& guid) : IAsset(guid) {}
};
} // namespace aln