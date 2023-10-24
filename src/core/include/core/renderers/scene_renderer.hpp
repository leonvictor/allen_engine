#pragma once

#include "../components/camera.hpp"
#include "../components/light.hpp"
#include "../components/skeletal_mesh_component.hpp"
#include "../components/static_mesh_component.hpp"
#include "../world_systems/render_system.hpp"

#include <entities/world_entity.hpp>
#include <graphics/render_engine.hpp>
#include <graphics/rendering/render_target.hpp>
#include <graphics/rendering/renderer.hpp>
#include <graphics/resources/image.hpp>
#include <graphics/viewport.hpp>
#include <graphics/window.hpp>

#include <tracy/Tracy.hpp>
#include <vulkan/vulkan.hpp>

#include <functional>

namespace aln
{

class SceneRenderer : public IRenderer
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

  private:
    Pipeline m_staticMeshesPipeline;
    Pipeline m_skeletalMeshesPipeline;
    Pipeline m_skyboxPipeline;

    // Per frame data
    resources::Buffer m_sceneDataBuffer;
    vk::DescriptorSetLayout m_sceneDataDescriptorSetLayout;
    vk::DescriptorSet m_sceneDataDescriptorSet;

    resources::Buffer m_lightComponentsBuffer;
    vk::DescriptorSet m_lightsDescriptorSet;

    // Model transforms storage buffer.
    /// @todo One per frame in flight ?
    resources::Buffer m_modelTransformsBuffer;
    vk::DescriptorSetLayout m_modelTransformsDescriptorSetLayout;
    vk::DescriptorSet m_modelTransformsDescriptorSet;

    resources::Buffer m_skinningBuffer;
    vk::DescriptorSetLayout m_skinningBufferDescriptorSetLayout;
    vk::DescriptorSet m_skinningBufferDescriptorSet;

  private:
    // TODO: Rename
    void CreateInternal(RenderEngine* pRenderEngine)
    {
        m_pRenderEngine = pRenderEngine;
        auto colorImageFormat = m_pRenderEngine->GetWindow()->GetSwapchain().GetImageFormat();
        // TODO: Update the viewport based on the imgui scene window size
        auto windowSize = m_pRenderEngine->GetWindow()->GetFramebufferSize();

        // --- Render Pass
        m_renderpass = RenderPass(m_pRenderEngine, windowSize.width, windowSize.height);
        m_renderpass.AddColorAttachment(colorImageFormat);
        m_renderpass.AddDepthAttachment();
        m_renderpass.AddColorResolveAttachment(colorImageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

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

        // --- Create the render targets

        auto cb = m_pRenderEngine->GetGraphicsPersistentCommandPool().GetCommandBuffer();

        constexpr auto renderTargetCount = RenderEngine::GetFrameQueueSize();
        m_renderTargets.resize(renderTargetCount);
        for (auto renderTargetIdx = 0; renderTargetIdx < renderTargetCount; ++renderTargetIdx)
        {
            auto& renderTarget = m_renderTargets[renderTargetIdx];

            // Resolve image
            renderTarget.m_resolveImage.Initialize(
                m_pRenderEngine,
                windowSize.width,
                windowSize.height,
                1,
                vk::SampleCountFlagBits::e1,
                colorImageFormat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eSampled);
            renderTarget.m_resolveImage.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
            renderTarget.m_resolveImage.AddView(vk::ImageAspectFlagBits::eColor);
            renderTarget.m_resolveImage.AddSampler();
            renderTarget.m_resolveImage.TransitionLayout(cb, vk::ImageLayout::eGeneral);
            renderTarget.m_resolveImage.CreateDescriptorSet();
            renderTarget.m_resolveImage.SetDebugName("Scene Renderer Target (" + std::to_string(renderTargetIdx) + ") - Resolve");

            // Multisampling image
            renderTarget.m_multisamplingImage.Initialize(
                m_pRenderEngine,
                windowSize.width,
                windowSize.height,
                1,
                m_pRenderEngine->GetMSAASamples(),
                colorImageFormat,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment);

            renderTarget.m_multisamplingImage.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
            renderTarget.m_multisamplingImage.AddView(vk::ImageAspectFlagBits::eColor);
            renderTarget.m_multisamplingImage.SetDebugName("Scene Renderer Target (" + std::to_string(renderTargetIdx) + ") - Multisampling");

            // Depth image
            renderTarget.m_depthImage.Initialize(
                m_pRenderEngine,
                windowSize.width,
                windowSize.height,
                1,
                m_pRenderEngine->GetMSAASamples(),
                m_pRenderEngine->FindDepthFormat(),
                vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment);
            renderTarget.m_depthImage.Allocate(vk::MemoryPropertyFlagBits::eDeviceLocal);
            renderTarget.m_depthImage.AddView(vk::ImageAspectFlagBits::eDepth);
            renderTarget.m_depthImage.SetDebugName("Scene Renderer Target (" + std::to_string(renderTargetIdx) + ") - Depth");

            // Framebuffer
            Vector<vk::ImageView> attachments = {
                renderTarget.m_multisamplingImage.GetVkView(),
                renderTarget.m_depthImage.GetVkView(),
                renderTarget.m_resolveImage.GetVkView(),
            };

            vk::FramebufferCreateInfo framebufferInfo = {
                .renderPass = m_renderpass.GetVkRenderPass(),
                .attachmentCount = static_cast<uint32_t>(attachments.size()),
                .pAttachments = attachments.data(),
                .width = windowSize.width,
                .height = windowSize.height,
                .layers = 1,
            };

            renderTarget.m_framebuffer = m_pRenderEngine->GetVkDevice().createFramebuffer(framebufferInfo).value;

            // Sync
            renderTarget.m_renderFinished = m_pRenderEngine->GetVkDevice().createSemaphore({}).value;
        }

        Queue::SubmissionRequest request;
        request.ExecuteCommandBuffer(cb);

        m_pRenderEngine->GetGraphicsQueue().Submit(request, vk::Fence{});

        Vector<vk::WriteDescriptorSet> writeDescriptorSets;

        // ---- Per frame resources
        m_sceneDataBuffer.Initialize(
            pRenderEngine,
            pRenderEngine->PadUniformBufferSize(sizeof(SceneGPUData)),
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

        m_sceneDataDescriptorSetLayout = pRenderEngine->GetVkDevice().createDescriptorSetLayout(sceneDataDescriptorSetLayoutCreateInfo).value;
        m_pRenderEngine->SetDebugUtilsObjectName(m_sceneDataDescriptorSetLayout, "Scene Data Descriptor Set Layout");

        m_sceneDataDescriptorSet = pRenderEngine->AllocateDescriptorSet(&m_sceneDataDescriptorSetLayout);
        m_pRenderEngine->SetDebugUtilsObjectName(m_sceneDataDescriptorSet, "Scene Data Descriptor Set");

        vk::DescriptorBufferInfo sceneDataBufferInfo = {
            .buffer = m_sceneDataBuffer.GetVkBuffer(),
            .offset = 0,
            .range = pRenderEngine->PadUniformBufferSize(sizeof(SceneGPUData)),
        };

        writeDescriptorSets.push_back({
            .dstSet = m_sceneDataDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &sceneDataBufferInfo,
        });

        // ---- Lights resources
        // TODO: Dynamic size
        m_lightComponentsBuffer.Initialize(
            pRenderEngine,
            16 + (5 * 48),
            vk::BufferUsageFlagBits::eStorageBuffer,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

        m_pRenderEngine->SetDebugUtilsObjectName(m_lightComponentsBuffer.GetVkBuffer(), "Renderer Lights Buffer");

        m_lightsDescriptorSet = pRenderEngine->AllocateDescriptorSet<Light>();
        pRenderEngine->SetDebugUtilsObjectName(m_lightsDescriptorSet, "Lights Descriptor Set");

        // TODO: How do we update the lights array ?
        vk::DescriptorBufferInfo lightsBufferInfo = {
            .buffer = m_lightComponentsBuffer.GetVkBuffer(),
            .offset = 0,
            .range = vk::WholeSize,
        };

        vk::WriteDescriptorSet writeDescriptor = {
            .dstSet = m_lightsDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &lightsBufferInfo,
        };

        writeDescriptorSets.push_back(writeDescriptor);

        // ---- Per-object resources
        /// @todo Automatically grow the buffer if more objects are added
        m_modelTransformsBuffer.Initialize(
            pRenderEngine,
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

        m_modelTransformsDescriptorSetLayout = pRenderEngine->GetVkDevice().createDescriptorSetLayout(info).value;
        m_pRenderEngine->SetDebugUtilsObjectName(m_modelTransformsDescriptorSetLayout, "Model Transforms Descriptor Set Layout");

        m_modelTransformsDescriptorSet = pRenderEngine->AllocateDescriptorSet(&m_modelTransformsDescriptorSetLayout);
        m_pRenderEngine->SetDebugUtilsObjectName(m_modelTransformsDescriptorSet, "Model Transforms Descriptor Set");

        vk::DescriptorBufferInfo transformsBufferInfo = {
            .buffer = m_modelTransformsBuffer.GetVkBuffer(),
            .offset = 0,
            .range = sizeof(Matrix4x4) * MAX_MODELS,
        };

        writeDescriptorSets.push_back({
            .dstSet = m_modelTransformsDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eStorageBuffer,
            .pBufferInfo = &transformsBufferInfo,
        });

        // ----- Skinning resources
        /// @todo What size ?
        m_skinningBuffer.Initialize(
            pRenderEngine,
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

        m_skinningBufferDescriptorSetLayout = pRenderEngine->GetVkDevice().createDescriptorSetLayout(skinningDescriptorSetLayoutCreateInfo).value;
        m_pRenderEngine->SetDebugUtilsObjectName(m_skinningBufferDescriptorSetLayout, "Skinning Buffer Descriptor Set Layout");

        m_skinningBufferDescriptorSet = pRenderEngine->AllocateDescriptorSet(&m_skinningBufferDescriptorSetLayout);
        m_pRenderEngine->SetDebugUtilsObjectName(m_skinningBufferDescriptorSet, "Skinning Buffer Descriptor Set");

        writeDescriptorSets.push_back({
            .dstSet = m_skinningBufferDescriptorSet,
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

        auto windowSize = m_pRenderEngine->GetWindow()->GetFramebufferSize();
        // ---------------
        // Static Meshes Pipeline
        // ---------------
        m_staticMeshesPipeline = Pipeline(m_pRenderEngine);
        m_staticMeshesPipeline.SetVertexType<Vertex>();
        m_staticMeshesPipeline.SetRenderPass(m_renderpass.GetVkRenderPass());
        m_staticMeshesPipeline.SetExtent({windowSize.width, windowSize.height});
        m_staticMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/shader.vert", vk::ShaderStageFlagBits::eVertex);
        m_staticMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/shader.frag", vk::ShaderStageFlagBits::eFragment);
        m_staticMeshesPipeline.AddPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(SkinnedMeshPushConstant)); // Note: This is necessary for now so that static/skinned meshes pipelines are compatible and we can bind descriptor sets once for both
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_sceneDataDescriptorSetLayout);
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_pRenderEngine->GetDescriptorSetLayout<aln::Light>());
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_pRenderEngine->GetDescriptorSetLayout<aln::Mesh>());
        m_staticMeshesPipeline.RegisterDescriptorLayout(m_modelTransformsDescriptorSetLayout);

        m_staticMeshesPipeline.Create("static_meshes_pipeline_cache_data.bin");
        m_pRenderEngine->SetDebugUtilsObjectName(m_staticMeshesPipeline.GetVkPipeline(), "Static Meshes Pipeline");
        m_pRenderEngine->SetDebugUtilsObjectName(m_staticMeshesPipeline.GetLayout(), "Static Meshes Pipeline Layout");

        // ---------------
        // Skeletal Meshes Pipeline
        // ---------------
        m_skeletalMeshesPipeline = Pipeline(m_pRenderEngine);
        m_skeletalMeshesPipeline.SetVertexType<SkinnedVertex>();
        m_skeletalMeshesPipeline.SetRenderPass(m_renderpass.GetVkRenderPass());
        m_skeletalMeshesPipeline.SetExtent({windowSize.width, windowSize.height});
        m_skeletalMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/skeletal_mesh.vert", vk::ShaderStageFlagBits::eVertex);
        m_skeletalMeshesPipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/shader.frag", vk::ShaderStageFlagBits::eFragment);
        m_skeletalMeshesPipeline.AddPushConstant(vk::ShaderStageFlagBits::eVertex, 0, sizeof(SkinnedMeshPushConstant));
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_sceneDataDescriptorSetLayout);
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pRenderEngine->GetDescriptorSetLayout<aln::Light>());
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_pRenderEngine->GetDescriptorSetLayout<aln::Mesh>());
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_modelTransformsDescriptorSetLayout);
        m_skeletalMeshesPipeline.RegisterDescriptorLayout(m_skinningBufferDescriptorSetLayout);

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
    void Initialize(RenderEngine* pRenderEngine) override
    {
        CreateInternal(pRenderEngine);
        CreatePipelines();
    }

    void Shutdown() override
    {
        m_staticMeshesPipeline.Shutdown();
        m_skeletalMeshesPipeline.Shutdown();
        //m_skyboxPipeline.Shutdown();

        for (auto& renderTarget : m_renderTargets)
        {
            m_pRenderEngine->GetVkDevice().destroyFramebuffer(renderTarget.m_framebuffer);
            renderTarget.m_depthImage.Shutdown();
            renderTarget.m_multisamplingImage.Shutdown();
            renderTarget.m_resolveImage.Shutdown();
            m_pRenderEngine->GetVkDevice().destroySemaphore(renderTarget.m_renderFinished);
        }

        m_pRenderEngine->GetVkDevice().destroyDescriptorSetLayout(m_skinningBufferDescriptorSetLayout);
        m_pRenderEngine->GetVkDevice().destroyDescriptorSetLayout(m_modelTransformsDescriptorSetLayout);
        m_pRenderEngine->GetVkDevice().destroyDescriptorSetLayout(m_sceneDataDescriptorSetLayout);

        m_skinningBuffer.Shutdown();
        m_lightComponentsBuffer.Shutdown();
        m_modelTransformsBuffer.Shutdown();
        m_sceneDataBuffer.Shutdown();

        m_renderpass.Shutdown();
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

    void RenderSkeletalMeshes(const Vector<const SkeletalMeshComponent*>& skeletalMeshComponents, vk::CommandBuffer& cb, uint32_t currentMeshIndex)
    {
        m_skeletalMeshesPipeline.Bind(cb);

        SkinnedMeshPushConstant pushConstant;
        uint32_t totalBonesCount = 0;

        const SkeletalMeshComponent* pMeshComponent = skeletalMeshComponents[0];
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
        size_t componentsCount = skeletalMeshComponents.size();
        for (size_t componentIndex = 1; componentIndex < componentsCount; ++componentIndex)
        {
            pMeshComponent = skeletalMeshComponents[componentIndex];
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

    void Render(WorldEntity* pWorld, TransientCommandBuffer& cb)
    {
        // Descriptors
        // 0 - Per frame (scene data (=viewproj matrix))
        // 1 - Per frame (lights) // TODO
        // 2 - Per mesh (vertices/indices)
        // 3 - Per mesh instance (transforms array) // STORAGE
        // 4 - (optionnal) Per skinned mesh skinning transforms // Storage

        ZoneScoped;

        const auto currentFrameIdx = m_pRenderEngine->GetCurrentFrameIdx();
        auto& renderTarget = m_renderTargets[currentFrameIdx];
        const auto& data = pWorld->GetSystem<GraphicsSystem>()->GetRenderData();

        RenderPass::Context renderPassCtx = {
            .commandBuffer = (vk::CommandBuffer&) cb,
            .framebuffer = renderTarget.m_framebuffer,
            .backgroundColor = data.m_pCameraComponent->m_backgroundColor,
        };

        m_renderpass.Begin(renderPassCtx);

        SceneGPUData sceneData = {
            .m_view = data.m_pCameraComponent->GetViewMatrix(),
            .m_projection = data.m_pCameraComponent->GetProjectionMatrix(pWorld->GetViewport()->GetAspectRatio()),
            .m_cameraPosition = data.m_pCameraComponent->GetLocalTransform().GetTranslation(),
        };

        // ---- Upload frame data
        m_sceneDataBuffer.Map();
        m_sceneDataBuffer.Copy(sceneData);
        m_sceneDataBuffer.Unmap();

        // ---- Update the lights buffer
        /// @todo: FIXME !
        int nLights = data.m_lightComponents.Size();
        m_lightComponentsBuffer.Map(0, sizeof(int)); // TODO: Respect spec alignment for int
        m_lightComponentsBuffer.Copy(nLights);
        m_lightComponentsBuffer.Unmap();

        int i = 0;
        for (auto pLight : data.m_lightComponents)
        {
            auto ubo = pLight->GetUniform();
            m_lightComponentsBuffer.Map(sizeof(int) + i * sizeof(LightUniform), sizeof(LightUniform));
            m_lightComponentsBuffer.Copy(ubo);
            m_lightComponentsBuffer.Unmap();
            i++;
        }

        // ---- Upload all transforms to the GPU storage buffer
        // TODO: Only upload transforms that changed ? It would require keeping the same index between render passes
        Matrix4x4 modelMatrix;
        auto pBufferMemory = m_modelTransformsBuffer.Map<Matrix4x4>();
        for (const auto pSkeletalMeshComponent : data.m_visibleSkeletalMeshComponents)
        {
            modelMatrix = pSkeletalMeshComponent->GetWorldTransform().ToMatrix();
            memcpy(pBufferMemory, &modelMatrix, sizeof(Matrix4x4));
            pBufferMemory++;
        }

        for (const auto pStaticMeshComponent : data.m_visibleStaticMeshComponents)
        {
            modelMatrix = pStaticMeshComponent->GetWorldTransform().ToMatrix();
            memcpy(pBufferMemory, &modelMatrix, sizeof(Matrix4x4));
            pBufferMemory++;
        }
        m_modelTransformsBuffer.Unmap();

        // ---- Upload skeletal mesh's transform matrices
        pBufferMemory = m_skinningBuffer.Map<Matrix4x4>();
        for (auto& pSkeletalMeshComponent : data.m_visibleSkeletalMeshComponents)
        {
            auto boneCount = pSkeletalMeshComponent->GetBonesCount();
            memcpy(pBufferMemory, pSkeletalMeshComponent->m_skinningTransforms.data(), boneCount * sizeof(Matrix4x4));
            pBufferMemory += boneCount;
        }
        m_skinningBuffer.Unmap();

        // Rendering
        /// @note: Descriptors are only bound to the skeletal meshes pipeline first because it's the first thing we render, but that could change
        m_skeletalMeshesPipeline.BindDescriptorSet(cb, m_sceneDataDescriptorSet, 0);
        m_skeletalMeshesPipeline.BindDescriptorSet(cb, m_lightsDescriptorSet, 1);
        m_skeletalMeshesPipeline.BindDescriptorSet(cb, m_modelTransformsDescriptorSet, 3);
        m_skeletalMeshesPipeline.BindDescriptorSet(cb, m_skinningBufferDescriptorSet, 4);

        uint32_t meshIndex = 0;

        // ---- Render skeletal meshes
        if (!data.m_visibleSkeletalMeshComponents.empty())
        {
            RenderSkeletalMeshes(data.m_visibleSkeletalMeshComponents, cb, meshIndex);
            meshIndex += data.m_visibleSkeletalMeshComponents.size();
        }

        // ---- Render static meshes
        if (!data.m_visibleStaticMeshComponents.empty())
        {
            RenderStaticMeshes(data.m_visibleStaticMeshComponents, cb, meshIndex);
            meshIndex += data.m_visibleSkeletalMeshComponents.size();
        }

        // ...

        cb->endRenderPass();
    }

    const RenderTarget* GetCurrentRenderTarget() const { return &m_renderTargets[m_pRenderEngine->GetCurrentFrameIdx()]; }
};
} // namespace aln