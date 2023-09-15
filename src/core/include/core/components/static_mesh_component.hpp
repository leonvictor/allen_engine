#pragma once

#include "../static_mesh.hpp"
#include "mesh_component.hpp"

namespace aln
{

/// @brief The StaticMeshComponent component holds a mesh, its material, and the vulkan objects
// representing them on the GPU.
class StaticMeshComponent : public MeshComponent
{
    ALN_REGISTER_TYPE();

  private:
    AssetHandle<StaticMesh> m_pMesh;

  public:
    inline const StaticMesh* GetMesh() const { return m_pMesh.get(); }

    static Vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings()
    {
        Vector<vk::DescriptorSetLayoutBinding> bindings{
            {
                // We need to access the ubo in the fragment shader aswell now (because it contains light direction)
                // TODO: There is probably a cleaner way (a descriptor for all light sources for example ?)

                // UBO
                .binding = 0,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                .pImmutableSamplers = nullptr,
            },
            {
                // Sampler
                .binding = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
                .pImmutableSamplers = nullptr,
            },
            {
                // Material
                .binding = 2,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eFragment,
            }};
        return bindings;
    }

  private:
    void Load(const LoadingContext& loadingContext) override
    {
        loadingContext.m_pAssetService->Load(m_pMesh);
    }

    void Unload(const LoadingContext& loadingContext) override
    {
        loadingContext.m_pAssetService->Unload(m_pMesh);
    }

    bool UpdateLoadingStatus() override
    {
        if (m_pMesh.IsLoaded())
        {
            m_status = Status::Loaded;
        }
        else if (!m_pMesh.IsValid() || m_pMesh.HasFailedLoading())
        {
            m_status = Status::LoadingFailed;
        }

        return IsLoaded();
    }
};
} // namespace aln