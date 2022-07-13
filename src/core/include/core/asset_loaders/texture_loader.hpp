#pragma once

#include <assets/asset.hpp>
#include <assets/loader.hpp>

#include "../texture.hpp"

#include <memory>

namespace aln
{

class TextureLoader : public IAssetLoader<Texture>
{
  private:
    std::shared_ptr<vkg::Device> m_pDevice;
    std::string m_defaultTexturePath;

  public:
    TextureLoader(std::shared_ptr<vkg::Device> pDevice, std::string defaultTexturePath)
    {
        m_pDevice = pDevice;
        m_defaultTexturePath = defaultTexturePath;
    }

    bool Load(AssetRecord* pRecord) override
    {
        // TODO: Abstract io to another lib
        // TODO: Allow loading to fail
        // TODO: Split image loading and vulkan texture creation
        // TODO: Handle 2D/3D Textures here

        // Vulkan objects loading happens during initialization as we do not handle threaded creation yet
        auto pTex = pRecord->GetAsset<Texture>();
        pTex->m_image = vkg::resources::Image::FromFile(m_pDevice, pTex->GetID());
        pTex->m_image.AddView();
        pTex->m_image.AddSampler();
        pTex->m_image.SetDebugName("Material Texture"); // todo: add id name
        return true;
    }

    void Unload(AssetRecord* pRecord) override
    {
        auto pTex = pRecord->GetAsset<Texture>();
        // TODO: Make sure reassignement is good enough.
        pTex->m_image = vkg::resources::Image();
    }
};

} // namespace aln