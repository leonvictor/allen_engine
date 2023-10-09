#pragma once

#include <graphics/pipeline.hpp>
#include <graphics/rendering/renderer.hpp>
#include <graphics/resources/buffer.hpp>
#include <common/maths/matrix4x4.hpp>
#include <common/containers/vector.hpp>

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
    vkg::Pipeline m_pipeline;

    vkg::resources::Buffer m_vertexBuffer;
    vkg::resources::Buffer m_viewProjectionUBO;
    vk::UniqueDescriptorSet m_descriptorSet;

  public:
    void Initialize(vkg::Device* pDevice, vkg::render::IRenderer* pRenderer)
    {
        auto bufferSize = MaxLinesPerDrawCall * sizeof(DebugVertex) * 2;

        // Create vertex buffer
        m_vertexBuffer = vkg::resources::Buffer(pDevice, bufferSize, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        m_vertexBuffer.Map(0, bufferSize);

        // Create UBO
        m_viewProjectionUBO = vkg::resources::Buffer(pDevice, sizeof(UBO), vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
        m_viewProjectionUBO.Map();

        // Create descriptor set
        m_descriptorSet = pDevice->AllocateDescriptorSet<LinesRenderState>();
        pDevice->SetDebugUtilsObjectName(m_descriptorSet.get(), "Lines Descriptor Set");

        vk::DescriptorBufferInfo viewProjectionUBOInfo =
            {
                .buffer = m_viewProjectionUBO.GetVkBuffer(),
                .offset = 0,
                .range = vk::WholeSize,
            };

        vk::WriteDescriptorSet writeDescriptorSet =
            {
                .dstSet = m_descriptorSet.get(),
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .pBufferInfo = &viewProjectionUBOInfo,
            };

        pDevice->GetVkDevice().updateDescriptorSets(1, &writeDescriptorSet, 0, nullptr);

        // ---------------
        // Line drawing pipeline (used for debug)
        // ---------------
        // @todo: Disable outside of debug mode ?
        m_pipeline = vkg::Pipeline(pDevice);
        m_pipeline.SetVertexType<DebugVertex>();
        m_pipeline.SetRenderPass(pRenderer->GetRenderPass().GetVkRenderPass());
        m_pipeline.SetExtent(pRenderer->GetExtent());
        m_pipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/debug_line.vert", vk::ShaderStageFlagBits::eVertex);
        m_pipeline.RegisterShader(std::string(DEFAULT_SHADERS_DIR) + "/debug_line.frag", vk::ShaderStageFlagBits::eFragment);
        m_pipeline.RegisterDescriptorLayout(pDevice->GetDescriptorSetLayout<LinesRenderState>());
        m_pipeline.SetPrimitiveTopology(vk::PrimitiveTopology::eLineList);
        m_pipeline.SetDepthTestWriteEnable(false, true, vk::CompareOp::eAlways);
        m_pipeline.Create("pipeline_cache_data.bin");

        pDevice->SetDebugUtilsObjectName(m_pipeline.GetVkPipeline(), "Debug Lines Pipeline");
    }

    void Shutdown()
    {
        m_vertexBuffer.Unmap();
        m_vertexBuffer = vkg::resources::Buffer();

        m_viewProjectionUBO.Unmap();
        m_viewProjectionUBO = vkg::resources::Buffer();
    }

    static Vector<vk::DescriptorSetLayoutBinding> GetDescriptorSetLayoutBindings()
    {
        Vector<vk::DescriptorSetLayoutBinding> bindings{
            {
                .binding = 0,
                .descriptorType = vk::DescriptorType::eUniformBuffer,
                .descriptorCount = 1,
                .stageFlags = vk::ShaderStageFlagBits::eVertex,
            }};

        return bindings;
    }
};
} // namespace aln