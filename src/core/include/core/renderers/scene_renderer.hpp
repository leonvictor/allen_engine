#pragma once

#include <graphics/device.hpp>
#include <graphics/rendering/render_target.hpp>
#include <graphics/rendering/renderer.hpp>
#include <graphics/resources/image.hpp>
#include <graphics/swapchain.hpp>

#include <Tracy.hpp>
#include <functional>
#include <vulkan/vulkan.hpp>

#include "../components/light.hpp"
#include "../components/skeletal_mesh_component.hpp"
#include "../components/static_mesh_component.hpp"

namespace aln
{

class SceneRenderer : public vkg::render::IRenderer
{
  public:
    struct SceneGPUData
    {
        alignas(16) glm::mat4 m_view;
        alignas(16) glm::mat4 m_projection;
        alignas(16) glm::vec3 m_cameraPosition;
    };

    struct SkinnedMeshPushConstant
    {
        uint32_t m_bonesCount;
        uint32_t m_bonesStartIndex;
    };

    struct RenderData
    {
        std::vector<const SkeletalMeshComponent*>& m_skeletalMeshComponents;
        std::vector<const StaticMeshComponent*>& m_staticMeshComponents;
        IDVector<Light*>& m_lights;
        SceneGPUData m_sceneData;
    };

  private:
    uint32_t m_nTargetImages;

    vkg::Pipeline m_staticMeshesPipeline;
    vkg::Pipeline m_skeletalMeshesPipeline;
    vkg::Pipeline m_skyboxPipeline;

    // Per frame data
    vkg::resources::Buffer m_sceneDataBuffer;
    vk::UniqueDescriptorSetLayout m_pSceneDataDescriptorSetLayout;
    vk::UniqueDescriptorSet m_pSceneDataDescriptorSet;

    vkg::resources::Buffer m_lightsBuffer;
    vk::UniqueDescriptorSet m_lightsDescriptorSet;

    // Model transforms storage buffer.
    /// @todo One per frame in flight ?
    vkg::resources::Buffer m_modelTransformsBuffer;
    vk::UniqueDescriptorSetLayout m_pModelTransformsDescriptorSetLayout;
    vk::UniqueDescriptorSet m_pModelTransformsDescriptorSet;

    vkg::resources::Buffer m_skinningBuffer;
    vk::UniqueDescriptorSetLayout m_pSkinningBufferDescriptorSetLayout;
    vk::UniqueDescriptorSet m_pSkinningBufferDescriptorSet;

    void CreateTargetImages() override
    {
        m_targetImages.clear();

        auto cbs = m_pDevice->GetGraphicsCommandPool().BeginSingleTimeCommands();

        for (uint8_t i = 0; i < m_nTargetImages; ++i)
        {
            auto target = std::make_shared<vkg::resources::Image>(
                m_pDevice, m_width, m_height, 1,
                vk::SampleCountFlagBits::e1,
                m_colorImageFormat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled);
            target->Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
            target->AddView(vk::ImageAspectFlagBits::eColor);
            target->AddSampler();
            target->SetDebugName("Offline Renderer Target");

            target->TransitionLayout(cbs[0], vk::ImageLayout::eGeneral);
            m_pDevice->SetDebugUtilsObjectName(target->GetVkImage(), "Offline Render Target Image (" + std::to_string(i) + ")");

            m_targetImages.push_back(target);
        }

        m_pDevice->GetGraphicsCommandPool().EndSingleTimeCommands(cbs);
    }

    vkg::render::RenderTarget& GetNextTarget() override
    {
        m_activeImageIndex++;
        if (m_activeImageIndex >= m_nTargetImages)
        {
            m_activeImageIndex = 0;
        }

        return m_renderTargets[m_activeImageIndex];
    }

    void CreateRenderpass() override
    {
        m_renderpass = vkg::RenderPass(m_pDevice, m_width, m_height);
        m_renderpass.AddColorAttachment(m_colorImageFormat);
        m_renderpass.AddDepthAttachment();
        m_renderpass.AddColorResolveAttachment(m_colorImageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

        auto& subpass = m_renderpass.AddSubpass();
        subpass.ReferenceColorAttachment(0);
        subpass.ReferenceDepthAttachment(1);
        subpass.ReferenceResolveAttachment(2);

        // Add a subpass dependency to ensure the render pass will wait for the right stage
        // We need to wait for the image to be acquired before transitionning to it
        vk::SubpassDependency dep;
        dep.srcSubpass = VK_SUBPASS_EXTERNAL;                                 // The implicit subpass before or after the render pass
        dep.dstSubpass = 0;                                                   // Target subpass index (we have only one)
        dep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput; // Stage to wait on
        dep.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        dep.srcAccessMask = vk::AccessFlagBits(0);
        dep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        m_renderpass.AddSubpassDependency(dep);
        m_renderpass.Create();
    }

    // TODO: Rename
    void CreateInternal(vkg::Device* pDevice, uint32_t width, uint32_t height, vk::Format colorImageFormat) override
    {
        vkg::render::IRenderer::CreateInternal(pDevice, width, height, colorImageFormat);

        std::vector<vk::WriteDescriptorSet> writeDescriptorSets;

        // ---- Per frame resources
        m_sceneDataBuffer = vkg::resources::Buffer(
            pDevice,
            pDevice->PadUniformBufferSize(sizeof(SceneGPUData)),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        vk::DescriptorSetLayoutBinding sceneDataBinding =
            {
                .binding = 0,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            };

        vk::DescriptorSetLayoutCreateInfo sceneDataDescriptorSetLayoutCreateInfo =
            {
                .bindingCount = 1,
                .pBindings = &sceneDataBinding,
            };

        m_pSceneDataDescriptorSetLayout = pDevice->GetVkDevice().createDescriptorSetLayoutUnique(sceneDataDescriptorSetLayoutCreateInfo);
        m_pSceneDataDescriptorSet = pDevice->AllocateDescriptorSet(&m_pSceneDataDescriptorSetLayout.get());

        vk::DescriptorBufferInfo sceneDataBufferInfo =
            {
                .buffer = m_sceneDataBuffer.GetVkBuffer(),
                .offset = 0,
                .range = pDevice->PadUniformBufferSize(sizeof(SceneGPUData)),
            };

        writeDescriptorSets.push_back(
            {
                .dstSet = m_pSceneDataDescriptorSet.get(),
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &sceneDataBufferInfo,
            });

        // ---- Lights resources
        // TODO: Dynamic size
        m_lightsBuffer = vkg::resources::Buffer(pDevice, 16 + (5 * 48), vk::BufferUsageFlagBits::eStorageBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        m_lightsDescriptorSet = pDevice->AllocateDescriptorSet<Light>();
        pDevice->SetDebugUtilsObjectName(m_lightsDescriptorSet.get(), "Lights Descriptor Set");

        vk::DescriptorBufferInfo lightsBufferInfo;
        lightsBufferInfo.buffer = m_lightsBuffer.GetVkBuffer(); // TODO: How do we update the lights array ?
        lightsBufferInfo.offset = 0;
        lightsBufferInfo.range = VK_WHOLE_SIZE;

        vk::WriteDescriptorSet writeDescriptor;
        writeDescriptor.dstSet = m_lightsDescriptorSet.get();
        writeDescriptor.dstBinding = 0;
        writeDescriptor.dstArrayElement = 0;
        writeDescriptor.descriptorType = vk::DescriptorType::eStorageBuffer;
        writeDescriptor.descriptorCount = 1;
        writeDescriptor.pBufferInfo = &lightsBufferInfo;

        writeDescriptorSets.push_back(writeDescriptor);

        // ---- Per-object resources
        /// @todo Automatically grow the buffer if more objects are added
        m_modelTransformsBuffer = vkg::resources::Buffer(
            pDevice,
            sizeof(glm::mat4x4) * MAX_MODELS,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        vk::DescriptorSetLayoutBinding binding =
            {
                .binding = 0,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex,
            };

        vk::DescriptorSetLayoutCreateInfo info =
            {
                .bindingCount = 1,
                .pBindings = &binding,
            };

        m_pModelTransformsDescriptorSetLayout = pDevice->GetVkDevice().createDescriptorSetLayoutUnique(info);
        m_pModelTransformsDescriptorSet = pDevice->AllocateDescriptorSet(&m_pModelTransformsDescriptorSetLayout.get());

        vk::DescriptorBufferInfo transformsBufferInfo =
            {
                .buffer = m_modelTransformsBuffer.GetVkBuffer(),
                .offset = 0,
                .range = sizeof(glm::mat4x4) * MAX_MODELS,
            };

        writeDescriptorSets.push_back(
            {
                .dstSet = m_pModelTransformsDescriptorSet.get(),
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &transformsBufferInfo,
            });

        // ----- Skinning resources
        /// @todo What size ?
        m_skinningBuffer = vkg::resources::Buffer(
            pDevice,
            MAX_SKINNING_TRANSFORMS * sizeof(glm::mat4x4),
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        vk::DescriptorBufferInfo skinningBufferInfo =
            {
                .buffer = m_skinningBuffer.GetVkBuffer(),
                .offset = 0,
                .range = VK_WHOLE_SIZE,
            };

        vk::DescriptorSetLayoutBinding skinningBufferBinding =
            {
                .binding = 0,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex,
            };

        vk::DescriptorSetLayoutCreateInfo skinningDescriptorSetLayoutCreateInfo =
            {
                .bindingCount = 1,
                .pBindings = &skinningBufferBinding,
            };

        m_pSkinningBufferDescriptorSetLayout = pDevice->GetVkDevice().createDescriptorSetLayoutUnique(skinningDescriptorSetLayoutCreateInfo);
        m_pSkinningBufferDescriptorSet = pDevice->AllocateDescriptorSet(&m_pSkinningBufferDescriptorSetLayout.get());

        writeDescriptorSets.push_back(
            {
                .dstSet = m_pSkinningBufferDescriptorSet.get(),
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eStorageBuffer,
                .pBufferInfo = &skinningBufferInfo,
            });

        m_pDevice->GetVkDevice().updateDescriptorSets(writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }

    void CreatePipelines()
    {
        // TODO: Handle default shader dir in case of separate projects
        // How do we bundle them ?

        // ---------------
        // Static Meshes Pipeline
        // ---------------
        m_staticMeshesPipeline = vkg::Pipeline(m_pDevice);
        m_staticMeshesPipeline.SetVertexType<Vertex>();
        m_staticMeshesPipeline.SetRenderPass(m_renderpass.GetVkRenderPass());
        m_staticMeshesPipeline.SetExtent({m_width, m_height});
        m_staticMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/shader.vert", vk::ShaderStageFlagBits::eVertex);
        m_staticMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/shader.frag", vk::ShaderStageFlagBits::eFragment);
        m_staticMeshesPipeline.AddPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(SkinnedMeshPushConstant)); // Note: This is necessary for now so that static/skinned meshes pipelines are compatible and we can bind descriptor sets once for both
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_pSceneDataDescriptorSetLayout.get());
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_pDevice->GetDescriptorSetLayout<aln::Light>());
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_pDevice->GetDescriptorSetLayout<aln::Mesh>());
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_pModelTransformsDescriptorSetLayout.get());
        
        m_staticMeshesPipeline.Create("static_meshes_pipeline_cache_data.bin");
        m_pDevice->SetDebugUtilsObjectName(m_staticMeshesPipeline.GetVkPipeline(), "Static Meshes Pipeline");

        // ---------------
        // Skeletal Meshes Pipeline
        // ---------------
        m_skeletalMeshesPipeline = vkg::Pipeline(m_pDevice);
        m_skeletalMeshesPipeline.SetVertexType<SkinnedVertex>();
        m_skeletalMeshesPipeline.SetRenderPass(m_renderpass.GetVkRenderPass());
        m_skeletalMeshesPipeline.SetExtent({m_width, m_height});
        m_skeletalMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/skeletal_mesh.vert", vk::ShaderStageFlagBits::eVertex);
        m_skeletalMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/shader.frag", vk::ShaderStageFlagBits::eFragment);
        m_skeletalMeshesPipeline.AddPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(SkinnedMeshPushConstant));
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pSceneDataDescriptorSetLayout.get());
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pDevice->GetDescriptorSetLayout<aln::Light>());
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pDevice->GetDescriptorSetLayout<aln::Mesh>());
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pModelTransformsDescriptorSetLayout.get());
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pSkinningBufferDescriptorSetLayout.get());

        m_skeletalMeshesPipeline.Create("skeletal_meshes_pipeline_cache_data.bin");
        m_pDevice->SetDebugUtilsObjectName(m_skeletalMeshesPipeline.GetVkPipeline(), "Skeletal Meshes Pipeline");

        // ---------------
        // Skybox Pipeline
        // ---------------
        // TODO: !! Put back skybox
        // m_skyboxPipeline = Pipeline(m_pDevice);
        // m_skyboxPipeline.SetRenderPass(m_renderpass.GetVkRenderPass());
        // m_skyboxPipeline.SetExtent({m_width, m_height});
        // m_skyboxPipeline.RegisterShader("shaders/skybox.vert", vk::ShaderStageFlagBits::eVertex);
        // m_skyboxPipeline.RegisterShader("shaders/skybox.frag", vk::ShaderStageFlagBits::eFragment);
        // m_skyboxPipeline.SetDepthTestWriteEnable(true, false);
        // m_skyboxPipeline.SetRasterizerCullMode(vk::CullModeFlagBits::eNone);
        // // m_skyboxPipeline.RegisterDescriptorLayout(m_pDevice->GetDescriptorSetLayout<StaticMeshComponent>());
        // m_skyboxPipeline.Create("skybox_pipeline_cache_data.bin");
        // m_pDevice->SetDebugUtilsObjectName(m_skyboxPipeline.GetVkPipeline(), "Skybox Pipeline");
    }

  public:
    void Create(vkg::Device* pDevice, int width, int height, int nTargetImages, vk::Format colorImageFormat)
    {
        m_nTargetImages = nTargetImages;
        CreateInternal(pDevice, width, height, colorImageFormat);
        CreatePipelines();
    }

    // TODO: De-duplicate
    void EndFrame()
    {
        auto& cb = m_renderTargets[m_activeImageIndex].commandBuffer.get();
        cb.endRenderPass();
        cb.end();

        vk::SubmitInfo submitInfo =
            {
                .commandBufferCount = 1,
                .pCommandBuffers = &cb,
            };

        m_pDevice->GetVkDevice().resetFences(m_frames[m_currentFrameIndex].inFlight.get());
        // TODO: It would be better to pass the semaphores and cbs directly to the queue class
        // but we need a mechanism to avoid having x versions of the method for (single elements * arrays * n_occurences)
        // vulkan arraywrappers ?
        m_pDevice->GetGraphicsQueue()
            .Submit(submitInfo, m_frames[m_currentFrameIndex].inFlight.get());

        m_currentFrameIndex = (m_currentFrameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void RenderStaticMeshes(const std::vector<const StaticMeshComponent*>& staticMeshComponents, vk::CommandBuffer& cb, uint32_t currentMeshIndex)
    {
        m_staticMeshesPipeline.Bind(cb);

        const Mesh* pCurrentMesh = staticMeshComponents[0]->GetMesh();
        cb.bindVertexBuffers(0, pCurrentMesh->GetVertexBuffer().GetVkBuffer(), vk::DeviceSize(0));
        cb.bindIndexBuffer(pCurrentMesh->GetIndexBuffer().GetVkBuffer(), 0, vk::IndexType::eUint32);

        m_staticMeshesPipeline.BindDescriptorSet(cb, pCurrentMesh->GetDescriptorSet(), 2);

        uint32_t firstInstance = currentMeshIndex;

        // This list has already gone through culling and is ordered by instance
        size_t componentsCount = staticMeshComponents.size();
        for (size_t componentIndex = 1; componentIndex < componentsCount; ++componentIndex)
        {
            const Mesh* pMesh = staticMeshComponents[componentIndex]->GetMesh();

            if (pCurrentMesh != pMesh)
            {
                // Draw instances of the current mesh
                const uint32_t instanceCount = currentMeshIndex - firstInstance + 1;
                cb.drawIndexed(pCurrentMesh->GetIndicesCount(), instanceCount, 0, 0, firstInstance);

                // Bind the next one
                pCurrentMesh = pMesh;

                m_staticMeshesPipeline.BindDescriptorSet(cb, pCurrentMesh->GetDescriptorSet(), 2);

                firstInstance = currentMeshIndex;
                cb.bindVertexBuffers(0, pCurrentMesh->GetVertexBuffer().GetVkBuffer(), vk::DeviceSize(0));
                cb.bindIndexBuffer(pCurrentMesh->GetIndexBuffer().GetVkBuffer(), 0, vk::IndexType::eUint32);
            }

            currentMeshIndex++;
        }

        const uint32_t instanceCount = currentMeshIndex - firstInstance + 1;
        cb.drawIndexed(pCurrentMesh->GetIndicesCount(), instanceCount, 0, 0, firstInstance);
    }

    void RenderSkeletalMeshes(const RenderData& data, vk::CommandBuffer& cb, uint32_t currentMeshIndex)
    {
        m_skeletalMeshesPipeline.Bind(cb);

        SkinnedMeshPushConstant pushConstant;
        uint32_t totalBonesCount = 0;

        const SkeletalMeshComponent* pMeshComponent = data.m_skeletalMeshComponents[0];
        const Mesh* pCurrentMesh = pMeshComponent->GetMesh();

        cb.bindVertexBuffers(0, pCurrentMesh->GetVertexBuffer().GetVkBuffer(), vk::DeviceSize(0));
        cb.bindIndexBuffer(pCurrentMesh->GetIndexBuffer().GetVkBuffer(), 0, vk::IndexType::eUint32);

        m_skeletalMeshesPipeline.BindDescriptorSet(cb, pCurrentMesh->GetDescriptorSet(), 2);

        pushConstant.m_bonesCount = pMeshComponent->GetBonesCount();
        pushConstant.m_bonesStartIndex = totalBonesCount;
        cb.pushConstants(m_skeletalMeshesPipeline.GetLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(SkinnedMeshPushConstant), &pushConstant);

        totalBonesCount += pushConstant.m_bonesCount;

        // This list has already gone through culling and is ordered by instance
        uint32_t firstInstance = currentMeshIndex;
        size_t componentsCount = data.m_skeletalMeshComponents.size();
        for (size_t componentIndex = 1; componentIndex < componentsCount; ++componentIndex)
        {
            pMeshComponent = data.m_skeletalMeshComponents[componentIndex];
            const Mesh* pMesh = pMeshComponent->GetMesh();

            if (pCurrentMesh != pMesh)
            {
                // Draw instances of the current mesh
                const uint32_t instanceCount = currentMeshIndex - firstInstance + 1;
                cb.drawIndexed(pCurrentMesh->GetIndicesCount(), instanceCount, 0, 0, firstInstance);

                // Bind the next one
                pCurrentMesh = pMesh;

                m_skeletalMeshesPipeline.BindDescriptorSet(cb, pCurrentMesh->GetDescriptorSet(), 2);

                pushConstant.m_bonesCount = pMeshComponent->GetBonesCount();
                pushConstant.m_bonesStartIndex = totalBonesCount;

                cb.pushConstants(m_skeletalMeshesPipeline.GetLayout(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(SkinnedMeshPushConstant), &pushConstant);
                // assert(constant.m_bonesCount < 255);

                totalBonesCount += pushConstant.m_bonesCount;

                firstInstance = currentMeshIndex;
                cb.bindVertexBuffers(0, pCurrentMesh->GetVertexBuffer().GetVkBuffer(), vk::DeviceSize(0));
                cb.bindIndexBuffer(pCurrentMesh->GetIndexBuffer().GetVkBuffer(), 0, vk::IndexType::eUint32);
            }

            currentMeshIndex++;
        }

        const uint32_t instanceCount = currentMeshIndex - firstInstance + 1;
        cb.drawIndexed(pCurrentMesh->GetIndicesCount(), instanceCount, 0, 0, firstInstance);
    }

    /// @todo: cb should be removable once all the rendering happens here
    virtual void Render(const RenderData& data, vk::CommandBuffer& cb)
    {
        // Descriptors
        // 0 - Per frame (scene data (=viewproj matrix))
        // 1 - Per frame (lights) // TODO
        // 2 - Per mesh (vertices/indices)
        // 3 - Per mesh instance (transforms array) // STORAGE
        // 4 - (optionnal) Per skinned mesh skinning transforms // Storage

        // ---- Upload frame data
        m_sceneDataBuffer.Map();
        m_sceneDataBuffer.Copy(data.m_sceneData);
        m_sceneDataBuffer.Unmap();

        // ---- Update the lights buffer
        /// @todo: FIXME !
        int nLights = data.m_lights.Size();
        m_lightsBuffer.Map(0, sizeof(int)); // TODO: Respect spec alignment for int
        m_lightsBuffer.Copy(nLights);
        m_lightsBuffer.Unmap();

        int i = 0;
        for (auto pLight : data.m_lights)
        {
            auto ubo = pLight->GetUniform();
            m_lightsBuffer.Map(sizeof(int) + i * sizeof(LightUniform), sizeof(LightUniform));
            m_lightsBuffer.Copy(ubo);
            m_lightsBuffer.Unmap();
            i++;
        }

        // ---- Upload all transforms to the GPU storage buffer
        // TODO: Only upload transforms that changed ? It would require keeping the same index between render passes
        glm::mat4x4 modelMatrix;
        auto pBufferMemory = m_modelTransformsBuffer.Map<glm::mat4x4>();
        for (const auto pSkeletalMeshComponent : data.m_skeletalMeshComponents)
        {
            modelMatrix = pSkeletalMeshComponent->GetWorldTransform().ToMatrix();
            memcpy(pBufferMemory, &modelMatrix, sizeof(glm::mat4x4));
            pBufferMemory++;
        }

        for (const auto pStaticMeshComponent : data.m_staticMeshComponents)
        {
            modelMatrix = pStaticMeshComponent->GetWorldTransform().ToMatrix();
            memcpy(pBufferMemory, &modelMatrix, sizeof(glm::mat4x4));
            pBufferMemory++;
        }
        m_modelTransformsBuffer.Unmap();

        // ---- Upload skeletal mesh's transform matrices
        pBufferMemory = m_skinningBuffer.Map<glm::mat4x4>();
        for (auto& pSkeletalMeshComponent : data.m_skeletalMeshComponents)
        {
            auto boneCount = pSkeletalMeshComponent->GetBonesCount();
            memcpy(pBufferMemory, pSkeletalMeshComponent->m_skinningTransforms.data(), boneCount * sizeof(glm::mat4x4));
            pBufferMemory += boneCount;
        }
        m_skinningBuffer.Unmap();

        // Rendering
        /// @note: Descriptors are only bound to the skeletal meshes pipeline first because it's the first thing we render, but that could change
        m_skeletalMeshesPipeline.BindDescriptorSet(cb, m_pSceneDataDescriptorSet.get(), 0);
        m_skeletalMeshesPipeline.BindDescriptorSet(cb, m_lightsDescriptorSet.get(), 1);
        m_skeletalMeshesPipeline.BindDescriptorSet(cb, m_pModelTransformsDescriptorSet.get(), 3);
        m_skeletalMeshesPipeline.BindDescriptorSet(cb, m_pSkinningBufferDescriptorSet.get(), 4);

        uint32_t meshIndex = 0;

        // ---- Render skeletal meshes
        if (!data.m_skeletalMeshComponents.empty())
        {
            RenderSkeletalMeshes(data, cb, meshIndex);
            meshIndex += data.m_skeletalMeshComponents.size();
        }

        

        // ---- Render static meshes
        if (!data.m_staticMeshComponents.empty())
        {
            RenderStaticMeshes(data.m_staticMeshComponents, cb, meshIndex);
            meshIndex += data.m_skeletalMeshComponents.size();
        }

        // ...
    }
};
} // namespace aln