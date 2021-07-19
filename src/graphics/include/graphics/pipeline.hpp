#pragma once

#include "device.hpp"
#include "shaders.hpp"

#include <iomanip>
#include <utils/uuid.hpp>
#include <vulkan/vulkan.hpp>

namespace aln::vkg
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

  public:
    vk::PipelineDepthStencilStateCreateInfo m_depthStencil;
    vk::PipelineRasterizationStateCreateInfo m_rasterizer;
    vk::PipelineMultisampleStateCreateInfo m_multisample;

    vk::Viewport m_viewport;
    vk::Rect2D m_scissor;

    Pipeline() {}

    explicit Pipeline(std::shared_ptr<Device> pDevice)
    {
        m_pDevice = pDevice;
        InitializeInternal();
    }

    /// @brief Copy the internal state of this pipeline. The other pipeline must not be initialized.
    void CopyTo(Pipeline& other) const;

    /// @brief Create a new pipeline copying the state of another one. The new pipeline will be unitialized.
    static Pipeline CopyFrom(const Pipeline& other)
    {
        Pipeline newPipeline = Pipeline(other.m_pDevice);
        other.CopyTo(newPipeline);
        return newPipeline;
    }

    /// @brief Initialize the pipeline and create the wrapped vulkan objects.
    void Create(std::string cachePath = "");

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
    void SetDepthTestWriteEnable(bool testEnable, bool writeEnable);

    void SetRasterizerCullMode(vk::CullModeFlagBits cullMode);

    void SetBindPoint(vk::PipelineBindPoint bindPoint);

    void Bind(vk::CommandBuffer& cb);

    /// @brief Bind a descriptor set to this pipeline.
    /// @todo: automatically handle the descriptor index
    /// We bind every frame, when are sets unbound ?
    void BindDescriptorSet(vk::CommandBuffer& cb, vk::DescriptorSet& descriptorSet, uint32_t index);

    inline bool IsInitialized() const { return m_status == State::Initialized; }
    inline vk::Pipeline& GetVkPipeline() { return m_vkPipeline.get(); }

  private:
    std::shared_ptr<Device> m_pDevice;
    vk::UniquePipelineLayout m_layout;
    vk::GraphicsPipelineCreateInfo m_pipelineCreateInfo;
    std::vector<vk::PipelineShaderStageCreateInfo> m_shaderStages;
    std::vector<vk::DescriptorSetLayout> m_descriptorSetLayouts;
    std::vector<vk::DynamicState> m_dynamicStates;
    vk::UniquePipeline m_vkPipeline;
    vk::PipelineBindPoint m_bindPoint;

    State m_status = State::Uninitialized;

    /// @brief Initialize default values
    void InitializeInternal();

    /// @brief Try to load a cached pipeline file. Based on https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/PipelineCache/PipelineCache.cpp
    /// @todo:
    /// - remove verbose output OR make it debug only
    vk::UniquePipelineCache LoadCachedPipeline(std::string path);
};
} // namespace aln::vkg