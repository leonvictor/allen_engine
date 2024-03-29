#pragma once

#include "render_engine.hpp"
#include "shaders.hpp"
#include "vertex_descriptors.hpp"

#include <common/uuid.hpp>

#include <vulkan/vulkan.hpp>

#include <iomanip>

namespace aln
{

/// @brief Wrapper around a vulkan pipeline.
class Pipeline
{
    enum State
    {
        // TODO: Handle states
        Uninitialized,
        Initialized,
        Bound,
    };

  private:
    RenderEngine* m_pRenderEngine;
    vk::PipelineLayout m_layout;
    vk::GraphicsPipelineCreateInfo m_pipelineCreateInfo;
    Vector<shaders::ShaderInfo> m_shaderStages;
    Vector<vk::DescriptorSetLayout> m_descriptorSetLayouts;
    Vector<vk::DynamicState> m_dynamicStates;
    Vector<vk::PushConstantRange> m_pushConstants;

    // Pipeline Input Assembly State
    vk::PrimitiveTopology m_primitiveTopology;

    vk::Pipeline m_pipeline;
    vk::PipelineBindPoint m_bindPoint;

    // Vertex description
    vk::VertexInputBindingDescription m_vertexBindingDescription;
    Vector<vk::VertexInputAttributeDescription> m_vertextAttributeDescriptions;

    State m_status = State::Uninitialized;

    /// @brief Initialize default values
    void InitializeInternal();

    /// @brief Try to load a cached pipeline file. Based on https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/PipelineCache/PipelineCache.cpp
    /// @todo:
    /// - remove verbose output OR make it debug only
    vk::PipelineCache LoadCachedPipeline(std::string& path);

  public:
    // TODO: Move that to private
    vk::PipelineDepthStencilStateCreateInfo m_depthStencil;
    vk::PipelineRasterizationStateCreateInfo m_rasterizer;
    vk::PipelineMultisampleStateCreateInfo m_multisample;

    vk::Viewport m_viewport;
    vk::Rect2D m_scissor;

    Pipeline() = default;
    Pipeline(RenderEngine* pRenderEngine)
    {
        m_pRenderEngine = pRenderEngine;
        InitializeInternal();
    }

    void Shutdown()
    {
        assert(m_pipeline);
        m_pRenderEngine->GetVkDevice().destroyPipeline(m_pipeline);
        m_pRenderEngine->GetVkDevice().destroyPipelineLayout(m_layout);
    }

    /// @brief Copy the internal state of this pipeline. The other pipeline must not be initialized.
    void CopyTo(Pipeline& other) const;

    /// @brief Create a new pipeline copying the state of another one. The new pipeline will be unitialized.
    static Pipeline CopyFrom(const Pipeline& other)
    {
        Pipeline newPipeline = Pipeline(other.m_pRenderEngine);
        other.CopyTo(newPipeline);
        return newPipeline;
    }

    /// @brief Initialize the pipeline and create the wrapped vulkan objects.
    void Create(std::string cachePath = "");

    void SetPrimitiveTopology(vk::PrimitiveTopology topology)
    {
        assert(!IsInitialized());
        m_primitiveTopology = topology;
    }

    /// @brief Set viewport and scissors to match a whole extent.
    void SetExtent(const vk::Extent2D& extent);

    void SetRenderPass(const vk::RenderPass& renderPass);

    /// @brief Load and add a shader to this pipeline.
    /// @param filename: shader file path (spirv or glsl).
    /// @param stage: shader stage (vertex, fragment...).
    /// @param entryPoint: name of the shader function used as entry point.
    void RegisterShader(const std::string& filename, vk::ShaderStageFlagBits stage, std::string entrypoint = "main");

    /// @brief Clear all registered shaders.
    void ClearShaders();

    void RegisterDescriptorLayout(vk::DescriptorSetLayout& descriptorSetLayout);

    void AddDynamicState(vk::DynamicState state);
    void SetDepthTestWriteEnable(bool testEnable, bool writeEnable, vk::CompareOp compareOp = vk::CompareOp::eLess);
    void SetDepthBoundsTestEnable(bool enable, float minDepthBounds, float maxDepthBounds);

    void SetRasterizerCullMode(vk::CullModeFlagBits cullMode);

    void SetBindPoint(vk::PipelineBindPoint bindPoint);

    template <typename T>
    void SetVertexType()
    {
        m_vertextAttributeDescriptions = VertexDescriptor<T>::GetAttributeDescription();
        m_vertexBindingDescription = VertexDescriptor<T>::GetBindingDescription();
    }

    void AddPushConstant(vk::ShaderStageFlagBits stage, uint32_t offset, uint32_t size)
    {
        m_pushConstants.push_back({stage, offset, size});
    }

    void Bind(vk::CommandBuffer cb);

    /// @brief Bind a descriptor set to this pipeline.
    /// @todo: automatically handle the descriptor index
    /// We bind every frame, when are sets unbound ?
    void BindDescriptorSet(vk::CommandBuffer cb, const vk::DescriptorSet& descriptorSet, uint32_t index);

    inline bool IsInitialized() const { return m_status == State::Initialized; }
    inline const vk::Pipeline& GetVkPipeline() const { return m_pipeline; }
    inline const vk::PipelineLayout& GetLayout() const { return m_layout; }
};
} // namespace aln