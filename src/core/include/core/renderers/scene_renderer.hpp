#pragma once

#include "../components/light.hpp"
#include "../components/skeletal_mesh_component.hpp"
#include "../components/static_mesh_component.hpp"

#include <graphics/render_engine.hpp>
#include <graphics/rendering/render_target.hpp>
#include <graphics/rendering/renderer.hpp>
#include <graphics/resources/image.hpp>
#include <graphics/swapchain.hpp>

#include <tracy/Tracy.hpp>
#include <vulkan/vulkan.hpp>

#include <functional>

namespace aln
{

class SceneRenderer : public render::IRenderer
{
  public:
    struct SceneGPUData
    {
        alignas(16) Matrix4x4 m_view;
        alignas(16) Matrix4x4 m_projection;
        alignas(16) Vec3 m_cameraPosition;
    };

    struct SkinnedMeshPushConstant
    {
        uint32_t m_bonesCount;
        uint32_t m_bonesStartIndex;
    };

    struct RenderData
    {
        Vector<const SkeletalMeshComponent*>& m_skeletalMeshComponents;
        Vector<const StaticMeshComponent*>& m_staticMeshComponents;
        IDVector<Light*>& m_lights;
        SceneGPUData m_sceneData;
    };

  private:
    uint32_t m_nTargetImages;

    Pipeline m_staticMeshesPipeline;
    Pipeline m_skeletalMeshesPipeline;
    Pipeline m_skyboxPipeline;

    // Per frame data
    resources::Buffer m_sceneDataBuffer;
    vk::UniqueDescriptorSetLayout m_pSceneDataDescriptorSetLayout;
    vk::UniqueDescriptorSet m_pSceneDataDescriptorSet;

    resources::Buffer m_lightsBuffer;
    vk::UniqueDescriptorSet m_pLightsDescriptorSet;

    // Model transforms storage buffer.
    /// @todo One per frame in flight ?
    resources::Buffer m_modelTransformsBuffer;
    vk::UniqueDescriptorSetLayout m_pModelTransformsDescriptorSetLayout;
    vk::UniqueDescriptorSet m_pModelTransformsDescriptorSet;

    resources::Buffer m_skinningBuffer;
    vk::UniqueDescriptorSetLayout m_pSkinningBufferDescriptorSetLayout;
    vk::UniqueDescriptorSet m_pSkinningBufferDescriptorSet;

    void CreateTargetImages() override
    {
        m_targetImages.clear();

        auto cbs = m_pRenderEngine->GetGraphicsCommandPool().BeginSingleTimeCommands();

        for (uint8_t i = 0; i < m_nTargetImages; ++i)
        {
            auto target = std::make_shared<resources::Image>(
                m_pRenderEngine, m_width, m_height, 1,
                vk::SampleCountFlagBits::e1,
                m_colorImageFormat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled);
            target->Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
            target->AddView(vk::ImageAspectFlagBits::eColor);
            target->AddSampler();
            target->TransitionLayout(cbs[0], vk::ImageLayout::eGeneral);

            target->SetDebugName("Scene Renderer Target (" + std::to_string(i) + ")");

            m_targetImages.push_back(target);
        }

        m_pRenderEngine->GetGraphicsCommandPool().EndSingleTimeCommands(cbs);
    }

    render::RenderTarget& GetNextTarget() override
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
        m_renderpass = RenderPass(m_pRenderEngine, m_width, m_height);
        m_renderpass.AddColorAttachment(m_colorImageFormat);
        m_renderpass.AddDepthAttachment();
        m_renderpass.AddColorResolveAttachment(m_colorImageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

        auto& subpass = m_renderpass.AddSubpass();
        subpass.ReferenceColorAttachment(0);
        subpass.ReferenceDepthAttachment(1);
        subpass.ReferenceResolveAttachment(2);

        // Add a subpass dependency to ensure the render pass will wait for the right stage
        // We need to wait for the image to be acquired before transitionning to it
        vk::SubpassDependency dep = {
            .srcSubpass = vk::SubpassExternal,
            .dstSubpass = 0,
            .srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
            .srcAccessMask = vk::AccessFlagBits::eNone,
            .dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite,
        };

        m_renderpass.AddSubpassDependency(dep);
        m_renderpass.Create();
    }

    // TODO: Rename
    void CreateInternal(RenderEngine* pDevice, uint32_t width, uint32_t height, vk::Format colorImageFormat) override
    {
        render::IRenderer::CreateInternal(pDevice, width, height, colorImageFormat);

        Vector<vk::WriteDescriptorSet> writeDescriptorSets;

        // ---- Per frame resources
        m_sceneDataBuffer = resources::Buffer(
            pDevice,
            pDevice->PadUniformBufferSize(sizeof(SceneGPUData)),
            vk::BufferUsageFlagBits::eUniformBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        m_pRenderEngine->SetDebugUtilsObjectName(m_sceneDataBuffer.GetVkBuffer(), "Renderer Scene Data Buffer");

        vk::DescriptorSetLayoutBinding sceneDataBinding = {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        };

        vk::DescriptorSetLayoutCreateInfo sceneDataDescriptorSetLayoutCreateInfo = {
            .bindingCount = 1,
            .pBindings = &sceneDataBinding,
        };

        m_pSceneDataDescriptorSetLayout = pDevice->GetVkDevice().createDescriptorSetLayoutUnique(sceneDataDescriptorSetLayoutCreateInfo).value;
        m_pRenderEngine->SetDebugUtilsObjectName(m_pSceneDataDescriptorSetLayout.get(), "Scene Data Descriptor Set Layout");

        m_pSceneDataDescriptorSet = pDevice->AllocateDescriptorSet(&m_pSceneDataDescriptorSetLayout.get());
        m_pRenderEngine->SetDebugUtilsObjectName(m_pSceneDataDescriptorSet.get(), "Scene Data Descriptor Set");

        vk::DescriptorBufferInfo sceneDataBufferInfo = {
            .buffer = m_sceneDataBuffer.GetVkBuffer(),
            .offset = 0,
            .range = pDevice->PadUniformBufferSize(sizeof(SceneGPUData)),
        };

        writeDescriptorSets.push_back({
            .dstSet = m_pSceneDataDescriptorSet.get(),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &sceneDataBufferInfo,
        });

        // ---- Lights resources
        // TODO: Dynamic size
        m_lightsBuffer = resources::Buffer(
            pDevice,
            16 + (5 * 48),
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        m_pRenderEngine->SetDebugUtilsObjectName(m_lightsBuffer.GetVkBuffer(), "Renderer Lights Buffer");

        m_pLightsDescriptorSet = pDevice->AllocateDescriptorSet<Light>();
        pDevice->SetDebugUtilsObjectName(m_pLightsDescriptorSet.get(), "Lights Descriptor Set");

        // TODO: How do we update the lights array ?
        vk::DescriptorBufferInfo lightsBufferInfo = {
            .buffer = m_lightsBuffer.GetVkBuffer(),
            .offset = 0,
            .range = vk::WholeSize,
        };

        vk::WriteDescriptorSet writeDescriptor = {
            .dstSet = m_pLightsDescriptorSet.get(),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &lightsBufferInfo,
        };

        writeDescriptorSets.push_back(writeDescriptor);

        // ---- Per-object resources
        /// @todo Automatically grow the buffer if more objects are added
        m_modelTransformsBuffer = resources::Buffer(
            pDevice,
            sizeof(Matrix4x4) * MAX_MODELS,
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        m_pRenderEngine->SetDebugUtilsObjectName(m_modelTransformsBuffer.GetVkBuffer(), "Renderer Model Transforms Buffer");

        vk::DescriptorSetLayoutBinding binding = {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
        };

        vk::DescriptorSetLayoutCreateInfo info = {
            .bindingCount = 1,
            .pBindings = &binding,
        };

        m_pModelTransformsDescriptorSetLayout = pDevice->GetVkDevice().createDescriptorSetLayoutUnique(info).value;
        m_pRenderEngine->SetDebugUtilsObjectName(m_pModelTransformsDescriptorSetLayout.get(), "Model Transforms Descriptor Set Layout");

        m_pModelTransformsDescriptorSet = pDevice->AllocateDescriptorSet(&m_pModelTransformsDescriptorSetLayout.get());
        m_pRenderEngine->SetDebugUtilsObjectName(m_pModelTransformsDescriptorSet.get(), "Model Transforms Descriptor Set");

        vk::DescriptorBufferInfo transformsBufferInfo = {
            .buffer = m_modelTransformsBuffer.GetVkBuffer(),
            .offset = 0,
            .range = sizeof(Matrix4x4) * MAX_MODELS,
        };

        writeDescriptorSets.push_back({
            .dstSet = m_pModelTransformsDescriptorSet.get(),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &transformsBufferInfo,
        });

        // ----- Skinning resources
        /// @todo What size ?
        m_skinningBuffer = resources::Buffer(
            pDevice,
            MAX_SKINNING_TRANSFORMS * sizeof(Matrix4x4),
            vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        m_pRenderEngine->SetDebugUtilsObjectName(m_skinningBuffer.GetVkBuffer(), "Renderer Skinning Buffer");

        vk::DescriptorBufferInfo skinningBufferInfo = {
            .buffer = m_skinningBuffer.GetVkBuffer(),
            .offset = 0,
            .range = vk::WholeSize,
        };

        vk::DescriptorSetLayoutBinding skinningBufferBinding = {
            .binding = 0,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
        };

        vk::DescriptorSetLayoutCreateInfo skinningDescriptorSetLayoutCreateInfo = {
            .bindingCount = 1,
            .pBindings = &skinningBufferBinding,
        };

        m_pSkinningBufferDescriptorSetLayout = pDevice->GetVkDevice().createDescriptorSetLayoutUnique(skinningDescriptorSetLayoutCreateInfo).value;
        m_pRenderEngine->SetDebugUtilsObjectName(m_pSkinningBufferDescriptorSetLayout.get(), "Skinning Buffer Descriptor Set Layout");

        m_pSkinningBufferDescriptorSet = pDevice->AllocateDescriptorSet(&m_pSkinningBufferDescriptorSetLayout.get());
        m_pRenderEngine->SetDebugUtilsObjectName(m_pSkinningBufferDescriptorSetLayout.get(), "Skinning Buffer Descriptor Set");

        writeDescriptorSets.push_back({
            .dstSet = m_pSkinningBufferDescriptorSet.get(),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &skinningBufferInfo,
        });

        m_pRenderEngine->GetVkDevice().updateDescriptorSets(writeDescriptorSets.size(), writeDescriptorSets.data(), 0, nullptr);
    }

    void CreatePipelines()
    {
        // TODO: Handle default shader dir in case of separate projects
        // How do we bundle them ?

        // ---------------
        // Static Meshes Pipeline
        // ---------------
        m_staticMeshesPipeline = Pipeline(m_pRenderEngine);
        m_staticMeshesPipeline.SetVertexType<Vertex>();
        m_staticMeshesPipeline.SetRenderPass(m_renderpass.GetVkRenderPass());
        m_staticMeshesPipeline.SetExtent({m_width, m_height});
        m_staticMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/shader.vert", vk::ShaderStageFlagBits::eVertex);
        m_staticMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/shader.frag", vk::ShaderStageFlagBits::eFragment);
        m_staticMeshesPipeline.AddPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(SkinnedMeshPushConstant)); // Note: This is necessary for now so that static/skinned meshes pipelines are compatible and we can bind descriptor sets once for both
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_pSceneDataDescriptorSetLayout.get());
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_pRenderEngine->GetDescriptorSetLayout<aln::Light>());
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_pRenderEngine->GetDescriptorSetLayout<aln::Mesh>());
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_pModelTransformsDescriptorSetLayout.get());

        m_staticMeshesPipeline.Create("static_meshes_pipeline_cache_data.bin");
        m_pRenderEngine->SetDebugUtilsObjectName(m_staticMeshesPipeline.GetVkPipeline(), "Static Meshes Pipeline");
        m_pRenderEngine->SetDebugUtilsObjectName(m_staticMeshesPipeline.GetLayout(), "Static Meshes Pipeline Layout");

        // ---------------
        // Skeletal Meshes Pipeline
        // ---------------
        m_skeletalMeshesPipeline = Pipeline(m_pRenderEngine);
        m_skeletalMeshesPipeline.SetVertexType<SkinnedVertex>();
        m_skeletalMeshesPipeline.SetRenderPass(m_renderpass.GetVkRenderPass());
        m_skeletalMeshesPipeline.SetExtent({m_width, m_height});
        m_skeletalMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/skeletal_mesh.vert", vk::ShaderStageFlagBits::eVertex);
        m_skeletalMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/shader.frag", vk::ShaderStageFlagBits::eFragment);
        m_skeletalMeshesPipeline.AddPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(SkinnedMeshPushConstant));
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pSceneDataDescriptorSetLayout.get());
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pRenderEngine->GetDescriptorSetLayout<aln::Light>());
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pRenderEngine->GetDescriptorSetLayout<aln::Mesh>());
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pModelTransformsDescriptorSetLayout.get());
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pSkinningBufferDescriptorSetLayout.get());

        m_skeletalMeshesPipeline.Create("skeletal_meshes_pipeline_cache_data.bin");
        m_pRenderEngine->SetDebugUtilsObjectName(m_skeletalMeshesPipeline.GetVkPipeline(), "Skeletal Meshes Pipeline");
        m_pRenderEngine->SetDebugUtilsObjectName(m_skeletalMeshesPipeline.GetLayout(), "Skeletal Meshes Pipeline Layout");

        // ---------------
        // Skybox Pipeline
        // ---------------
        // TODO: !! Put back skybox
        // m_skyboxPipeline = Pipeline(m_pRenderEngine);
        // m_skyboxPipeline.SetRenderPass(m_renderpass.GetVkRenderPass());
        // m_skyboxPipeline.SetExtent({m_width, m_height});
        // m_skyboxPipeline.RegisterShader("shaders/skybox.vert", vk::ShaderStageFlagBits::eVertex);
        // m_skyboxPipeline.RegisterShader("shaders/skybox.frag", vk::ShaderStageFlagBits::eFragment);
        // m_skyboxPipeline.SetDepthTestWriteEnable(true, false);
        // m_skyboxPipeline.SetRasterizerCullMode(vk::CullModeFlagBits::eNone);
        // // m_skyboxPipeline.RegisterDescriptorLayout(m_pRenderEngine->GetDescriptorSetLayout<StaticMeshComponent>());
        // m_skyboxPipeline.Create("skybox_pipeline_cache_data.bin");
        // m_pRenderEngine->SetDebugUtilsObjectName(m_skyboxPipeline.GetVkPipeline(), "Skybox Pipeline");
    }

  public:
    void Initialize(RenderEngine* pDevice, uint32_t width, uint32_t height, int nTargetImages, vk::Format colorImageFormat)
    {
        m_nTargetImages = nTargetImages;
        CreateInternal(pDevice, width, height, colorImageFormat);
        CreatePipelines();
    }

    void Shutdown() override
    {
        m_staticMeshesPipeline.Shutdown();
        m_skeletalMeshesPipeline.Shutdown();
        m_skyboxPipeline.Shutdown();

        for (auto& image : m_targetImages)
        {
            image->Shutdown();
        }

        m_pSkinningBufferDescriptorSet.reset();
        m_pModelTransformsDescriptorSet.reset();
        m_pSceneDataDescriptorSet.reset();
        m_pLightsDescriptorSet.reset();

        m_pSkinningBufferDescriptorSetLayout.reset();
        m_pModelTransformsDescriptorSetLayout.reset();
        m_pSceneDataDescriptorSetLayout.reset();

        m_skinningBuffer.Shutdown();
        m_lightsBuffer.Shutdown();
        m_modelTransformsBuffer.Shutdown();
        m_sceneDataBuffer.Shutdown();

        IRenderer::Shutdown();
    }

    // TODO: De-duplicate
    void EndFrame()
    {
        auto& cb = m_renderTargets[m_activeImageIndex].commandBuffer.get();
        cb.endRenderPass();
        cb.end();

        vk::SubmitInfo submitInfo = {
            .commandBufferCount = 1,
            .pCommandBuffers = &cb,
        };

        const auto& currentFrameIdx = m_pRenderEngine->GetCurrentFrameIdx();
        m_pRenderEngine->GetVkDevice().resetFences(m_frameSync[currentFrameIdx].inFlight.get());
        m_pRenderEngine->GetGraphicsQueue().Submit(submitInfo, m_frameSync[currentFrameIdx].inFlight.get());
    }

    void RenderStaticMeshes(const Vector<const StaticMeshComponent*>& staticMeshComponents, vk::CommandBuffer& cb, uint32_t currentMeshIndex)
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
        Matrix4x4 modelMatrix;
        auto pBufferMemory = m_modelTransformsBuffer.Map<Matrix4x4>();
        for (const auto pSkeletalMeshComponent : data.m_skeletalMeshComponents)
        {
            modelMatrix = pSkeletalMeshComponent->GetWorldTransform().ToMatrix();
            memcpy(pBufferMemory, &modelMatrix, sizeof(Matrix4x4));
            pBufferMemory++;
        }

        for (const auto pStaticMeshComponent : data.m_staticMeshComponents)
        {
            modelMatrix = pStaticMeshComponent->GetWorldTransform().ToMatrix();
            memcpy(pBufferMemory, &modelMatrix, sizeof(Matrix4x4));
            pBufferMemory++;
        }
        m_modelTransformsBuffer.Unmap();

        // ---- Upload skeletal mesh's transform matrices
        pBufferMemory = m_skinningBuffer.Map<Matrix4x4>();
        for (auto& pSkeletalMeshComponent : data.m_skeletalMeshComponents)
        {
            auto boneCount = pSkeletalMeshComponent->GetBonesCount();
            memcpy(pBufferMemory, pSkeletalMeshComponent->m_skinningTransforms.data(), boneCount * sizeof(Matrix4x4));
            pBufferMemory += boneCount;
        }
        m_skinningBuffer.Unmap();

        // Rendering
        /// @note: Descriptors are only bound to the skeletal meshes pipeline first because it's the first thing we render, but that could change
        m_skeletalMeshesPipeline.BindDescriptorSet(cb, m_pSceneDataDescriptorSet.get(), 0);
        m_skeletalMeshesPipeline.BindDescriptorSet(cb, m_pLightsDescriptorSet.get(), 1);
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