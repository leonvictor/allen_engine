#pragma once

#include <common/containers/vector.hpp>
#include <common/maths/matrix4x4.hpp>
#include <graphics/pipeline.hpp>
#include <graphics/rendering/renderer.hpp>
#include <graphics/resources/buffer.hpp>

namespace aln
{

class LinesRenderState
{
    friend class GraphicsSystem;

    static constexpr uint32_t MaxLinesPerDrawCall = 10000;

    struct UBO
    {
        Matrix4x4 m_viewProjectionMatrix;
    };

  private:
    Pipeline m_pipeline;

    resources::Buffer m_vertexBuffer;
    resources::Buffer m_viewProjectionUBO;
    vk::UniqueDescriptorSet m_descriptorSet;

  public:
    void Initialize(RenderEngine* pRenderEngine, IRenderer* pRenderer)
    {
        auto bufferSize = MaxLinesPerDrawCall * sizeof(DebugVertex) * 2;

        // Create vertex buffer
        m_vertexBuffer.Initialize(pRenderEngine, bufferSize, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        m_vertexBuffer.Map(0, bufferSize);
        pRenderEngine->SetDebugUtilsObjectName(m_vertexBuffer.GetVkBuffer(), "Debug Drawing Vertex Buffer");

        // Create UBO
        m_viewProjectionUBO.Initialize(pRenderEngine, sizeof(UBO), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        m_viewProjectionUBO.Map();
        pRenderEngine->SetDebugUtilsObjectName(m_vertexBuffer.GetVkBuffer(), "Debug Drawing UBO");

        // Create descriptor set
        m_descriptorSet = pRenderEngine->AllocateDescriptorSet<LinesRenderState>();
        pRenderEngine->SetDebugUtilsObjectName(m_descriptorSet.get(), "Lines Descriptor Set");

        vk::DescriptorBufferInfo viewProjectionUBOInfo = {
            .buffer = m_viewProjectionUBO.GetVkBuffer(),
            .offset = 0,
            .range = vk::WholeSize,
        };

        vk::WriteDescriptorSet writeDescriptorSet = {
            .dstSet = m_descriptorSet.get(),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &viewProjectionUBOInfo,
        };

        pRenderEngine->GetVkDevice().updateDescriptorSets(1, &writeDescriptorSet, 0, nullptr);

        // ---------------
        // Line drawing pipeline (used for debug)
        // ---------------
        // @todo: Disable outside of debug mode ?
        m_pipeline = Pipeline(pRenderEngine);
        m_pipeline.SetVertexType<DebugVertex>();
        m_pipeline.SetRenderPass(pRenderer->GetRenderPass().GetVkRenderPass());
        //m_pipeline.SetExtent(pRenderer->GetExtent()); TODO
        m_pipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/debug_line.vert", vk::ShaderStageFlagBits::eVertex);
        m_pipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/debug_line.frag", vk::ShaderStageFlagBits::eFragment);
        m_pipeline.RegisterDescriptorLayout(pRenderEngine->GetDescriptorSetLayout<LinesRenderState>());
        m_pipeline.SetPrimitiveTopology(vk::PrimitiveTopology::eLineList);
        m_pipeline.SetDepthTestWriteEnable(false, true, vk::CompareOp::eAlways);
        m_pipeline.Create("pipeline_cache_data.bin");

        pRenderEngine->SetDebugUtilsObjectName(m_pipeline.GetVkPipeline(), "Debug Lines Pipeline");
    }

    void Shutdown()
    {
        m_vertexBuffer.Unmap();
        m_vertexBuffer.Shutdown();

        m_viewProjectionUBO.Unmap();
        m_viewProjectionUBO.Shutdown();
    }

    static Vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings()
    {
        Vector<vk::DescriptorSetLayoutBinding> bindings = {{
            .binding = 0,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .descriptorCount = 1,
            .stageFlags = vk::ShaderStageFlagBits::eVertex,
        }};

        return bindings;
    }
};
} // namespace aln