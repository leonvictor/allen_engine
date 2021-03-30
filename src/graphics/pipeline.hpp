#pragma once

#include "../utils/uuid.cpp"
#include "../vertex.hpp"
#include "device.hpp"
#include "shaders.hpp"

#include <iomanip>
#include <vulkan/vulkan.hpp>

namespace vkg
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
    void CopyTo(Pipeline& other) const
    {
        assert(!other.IsInitialized());
        other.m_pDevice = m_pDevice;
        other.m_pipelineCreateInfo = m_pipelineCreateInfo;
        other.m_shaderStages = m_shaderStages;
        other.m_dynamicStates = m_dynamicStates;
        other.m_bindPoint = m_bindPoint;
    }

    /// @brief Create a new pipeline copying the state of another one. The new pipeline will be unitialized.
    static Pipeline CopyFrom(const Pipeline& other)
    {
        Pipeline newPipeline = Pipeline(other.m_pDevice);
        other.CopyTo(newPipeline);
        return newPipeline;
    }

    /// @brief Initialize the pipeline and create the wrapped vulkan objects.
    void Create(std::string cachePath = "")
    {
        assert(m_status == State::Uninitialized);

        auto vertextAttributeDescriptions = Vertex::getAttributeDescription();
        auto vertexBindingDescription = Vertex::getBindingDescription();

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {
            .vertexBindingDescriptionCount = 1,
            .pVertexBindingDescriptions = &vertexBindingDescription,
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(vertextAttributeDescriptions.size()),
            .pVertexAttributeDescriptions = vertextAttributeDescriptions.data(),
        };

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo = {
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = VK_FALSE,
        };

        // TODO: Handle multiple viewports/scissors
        vk::PipelineViewportStateCreateInfo viewportStateInfo = {
            .viewportCount = 1,
            .pViewports = &m_viewport,
            .scissorCount = 1,
            .pScissors = &m_scissor,
        };

        // Color blending = How do we combine the color returned by the fragment shader and the one that is already in the pixel ?
        // Configuration per attached framebuffer
        // Alpha blending -> the new color is blended with the old one based on its opacity :
        // finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor
        // finalColor.a = newAlpha.a
        vk::PipelineColorBlendAttachmentState colorBlendAttachment = {
            .blendEnable = VK_FALSE, // Disabled for now,
            .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
            .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
            .colorBlendOp = vk::BlendOp::eAdd,
            .srcAlphaBlendFactor = vk::BlendFactor::eOne,
            .dstAlphaBlendFactor = vk::BlendFactor::eZero,
            .alphaBlendOp = vk::BlendOp::eAdd,
            .colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
        };

        // Configuration for global color blending settings.
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo = {
            .logicOpEnable = VK_FALSE,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
            // We can also define custom blend constants in colorBlendInfo.blendConstants[0, 1, 2, 3]
        };

        // Not used for now
        // TODO: Pull out ?
        vk::PipelineLayoutCreateInfo layoutInfo = {
            .setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayouts.size()), // Update when we have more layouts
            .pSetLayouts = m_descriptorSetLayouts.data(),
        };

        m_layout = m_pDevice->GetVkDevice().createPipelineLayoutUnique(layoutInfo);

        // Shader stages
        m_pipelineCreateInfo.stageCount = m_shaderStages.size();
        m_pipelineCreateInfo.pStages = m_shaderStages.data();

        // Fixed-functions states
        m_pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
        m_pipelineCreateInfo.pColorBlendState = &colorBlendInfo;
        m_pipelineCreateInfo.pViewportState = &viewportStateInfo;
        // m_pipelineCreateInfo.pMultisampleState = &multisampleInfo;
        m_pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;

        vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
            .dynamicStateCount = static_cast<uint32_t>(m_dynamicStates.size()),
            .pDynamicStates = m_dynamicStates.data(),
        };

        m_pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

        // Pipeline layout
        m_pipelineCreateInfo.layout = m_layout.get();

        // Render pass
        m_pipelineCreateInfo.subpass = 0;
        m_pipelineCreateInfo.basePipelineHandle = vk::Pipeline();

        // TODO: Generate different pipeline cache path depending on the options
        vk::UniquePipelineCache pipelineCache = LoadCachedPipeline(cachePath);                                           // TODO
        m_vkPipeline = m_pDevice->GetVkDevice().createGraphicsPipelineUnique(pipelineCache.get(), m_pipelineCreateInfo); // TODO

        // TODO: Avoid saving if not needed
        // Store away the cache that we've populated.  This could conceivably happen
        // earlier, depends on when the pipeline cache stops being populated
        // internally.
        std::vector<uint8_t> endCacheData = m_pDevice->GetVkDevice().getPipelineCacheData(pipelineCache.get());

        // Write the file to disk, overwriting whatever was there
        std::ofstream writeCacheStream(cachePath, std::ios_base::out | std::ios_base::binary);
        if (writeCacheStream.good())
        {
            writeCacheStream.write(reinterpret_cast<char const*>(endCacheData.data()), endCacheData.size());
            writeCacheStream.close();
            std::cout << "  cacheData written to " << cachePath << "\n";
        }
        else
        {
            // Something bad happened
            std::cout << "  Unable to write cache data to disk!\n";
        }

        ClearShaders();
        m_dynamicStates.clear();
        m_descriptorSetLayouts.clear();
        m_status = State::Initialized;
    }

    /// @brief Set viewport and scissors to match a whole extent.
    void
    SetExtent(const vk::Extent2D& extent)
    {
        assert(!IsInitialized());
        m_viewport.width = extent.width;
        m_viewport.height = extent.height;
        m_scissor.extent = extent;
    }

    void SetRenderPass(const vk::RenderPass& renderPass)
    {
        assert(!IsInitialized());
        m_pipelineCreateInfo.renderPass = renderPass;
    }

    /// @brief Load and add a shader to this pipeline.
    /// @param filename: shader file path (spirv or glsl).
    /// @param stage: shader stage (vertex, fragment...).
    /// @param entryPoint: name of the shader function used as entry point.
    void RegisterShader(const std::string& filename, vk::ShaderStageFlagBits stage, std::string entrypoint = "main")
    {
        auto shaderStage = vkg::shaders::LoadShader(m_pDevice, filename, stage, entrypoint);
        m_shaderStages.push_back(shaderStage);
    }

    /// @brief Clear all registered shaders.
    void ClearShaders()
    {
        // TODO: Let the Shaders auto-destroy when they go out of scope
        for (const auto& shader : m_shaderStages)
        {
            m_pDevice->GetVkDevice().destroyShaderModule(shader.module);
        }
        m_shaderStages.clear();
    }

    void RegisterDescriptorLayout(vk::DescriptorSetLayout& descriptorSetLayout)
    {
        m_descriptorSetLayouts.push_back(descriptorSetLayout);
    }

    void AddDynamicState(vk::DynamicState state)
    {
        assert(!IsInitialized());
        m_dynamicStates.push_back(state);
    }

    void SetDepthTestWriteEnable(bool testEnable, bool writeEnable)
    {
        assert(!IsInitialized());
        m_depthStencil.depthTestEnable = testEnable ? VK_TRUE : VK_FALSE;
        m_depthStencil.depthWriteEnable = writeEnable ? VK_TRUE : VK_FALSE;
    }

    void SetRasterizerCullMode(vk::CullModeFlagBits cullMode)
    {
        assert(!IsInitialized());
        m_rasterizer.cullMode = cullMode;
    }

    void SetBindPoint(vk::PipelineBindPoint bindPoint)
    {
        assert(!IsInitialized());
        m_bindPoint = bindPoint;
    }

    void Bind(vk::CommandBuffer& cb)
    {
        assert(IsInitialized());
        cb.bindPipeline(m_bindPoint, m_vkPipeline.get());
    }

    /// @brief Bind a descriptor set to this pipeline.
    /// @todo: automatically handle the descriptor index
    /// We bind every frame, when are sets unbound ?
    void BindDescriptorSet(vk::CommandBuffer& cb, vk::DescriptorSet& descriptorSet, uint32_t index)
    {
        // TODO: firstSet and offsets.
        cb.bindDescriptorSets(m_bindPoint, m_layout.get(), index, descriptorSet, nullptr);
    }

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
    void InitializeInternal()
    {
        m_viewport.x = 0.0f;
        m_viewport.y = 0.0f;
        m_viewport.width = 0.0f;
        m_viewport.height = 0.0f;
        m_viewport.minDepth = 0.0f;
        m_viewport.maxDepth = 1.0f;

        m_multisample = {
            .rasterizationSamples = m_pDevice->GetMSAASamples(),
            .sampleShadingEnable = VK_TRUE,
            .minSampleShading = .2f,
        };

        m_scissor.offset = vk::Offset2D{0, 0};
        m_scissor.extent = vk::Extent2D{0, 0};

        // Initialize some parts with default values. We can modify them before calling create()
        // in order to specify some options.
        // TODO: Move that out. Default values belong in a constructor ?
        // Dynamic state: some of the states can be modified without recreating a pipeline, but will require arguments to be provided at draw time.
        m_depthStencil = {
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = vk::CompareOp::eLess, // Lower depth = closer
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE, // Not used
            .minDepthBounds = 0.0f,
            .maxDepthBounds = 1.0f,
        };

        // Rasterizer turns the geometry shaped by the vertices into fragments to be colored by the fragment shader.
        m_rasterizer = {
            .depthClampEnable = VK_FALSE,        // Whether to clamp rather than discard vertices that are beyond the near and far planes.
            .rasterizerDiscardEnable = VK_FALSE, // Disable any output to the framebuffer
            .polygonMode = vk::PolygonMode::eFill,
            .cullMode = vk::CullModeFlagBits::eBack, // Type of face culling
            .frontFace = vk::FrontFace::eCounterClockwise,
            // The rasterizer can alter depth values.
            .depthBiasEnable = VK_FALSE,
            .lineWidth = 1.0f,
        };

        // TODO: set this at creation time ?
        m_pipelineCreateInfo = {
            .pRasterizationState = &m_rasterizer,
            .pMultisampleState = &m_multisample,
            .pDepthStencilState = &m_depthStencil,
        };

        m_bindPoint = vk::PipelineBindPoint::eGraphics;
    }

    /// @brief Try to load a cached pipeline file. Based on https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/PipelineCache/PipelineCache.cpp
    /// @todo:
    /// - remove verbose output OR make it debug only
    vk::UniquePipelineCache LoadCachedPipeline(std::string path)
    {
        // Check disk for existing cache data
        size_t startCacheSize = 0;
        char* startCacheData = nullptr;
        std::ifstream readCacheStream(path, std::ios_base::in | std::ios_base::binary);
        if (readCacheStream.good())
        {
            // Determine cache size
            readCacheStream.seekg(0, readCacheStream.end);
            startCacheSize = static_cast<size_t>(readCacheStream.tellg());
            readCacheStream.seekg(0, readCacheStream.beg);

            // Allocate memory to hold the initial cache data
            startCacheData = (char*) std::malloc(startCacheSize);

            // Read the data into our buffer
            readCacheStream.read(startCacheData, startCacheSize);

            // Clean up and print results
            readCacheStream.close();
            std::cout << "  Pipeline cache HIT!\n";
            std::cout << "  cacheData loaded from " << path << "\n";
        }
        else
        {
            // No cache found on disk
            std::cout << "  Pipeline cache miss!\n";
        }

        if (startCacheData)
        {
            uint32_t headerLength = 0;
            uint32_t cacheHeaderVersion = 0;
            uint32_t vendorID = 0;
            uint32_t deviceID = 0;
            uint8_t pipelineCacheUUID[VK_UUID_SIZE] = {};

            memcpy(&headerLength, (uint8_t*) startCacheData + 0, 4);
            memcpy(&cacheHeaderVersion, (uint8_t*) startCacheData + 4, 4);
            memcpy(&vendorID, (uint8_t*) startCacheData + 8, 4);
            memcpy(&deviceID, (uint8_t*) startCacheData + 12, 4);
            memcpy(pipelineCacheUUID, (uint8_t*) startCacheData + 16, VK_UUID_SIZE);

            // Check each field and report bad values before freeing existing cache
            bool badCache = false;
            if (headerLength == 0)
            {
                badCache = true;
                std::cout << "  Bad header length in " << path << ".\n";
                std::cout << "    Cache contains: " << std::hex << std::setw(8) << headerLength << "\n";
            }
            if (cacheHeaderVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE)
            {
                badCache = true;
                std::cout << "  Unsupported cache header version in " << path << ".\n";
                std::cout << "    Cache contains: " << std::hex << std::setw(8) << cacheHeaderVersion << "\n";
            }
            if (vendorID != m_pDevice->GetPhysicalDeviceProperties().vendorID)
            {
                badCache = true;
                std::cout << "  Vender ID mismatch in " << path << ".\n";
                std::cout << "    Cache contains: " << std::hex << std::setw(8) << vendorID << "\n";
                std::cout << "    Driver expects: " << std::hex << std::setw(8) << m_pDevice->GetPhysicalDeviceProperties().vendorID << "\n";
            }
            if (deviceID != m_pDevice->GetPhysicalDeviceProperties().deviceID)
            {
                badCache = true;
                std::cout << "  Device ID mismatch in " << path << ".\n";
                std::cout << "    Cache contains: " << std::hex << std::setw(8) << deviceID << "\n";
                std::cout << "    Driver expects: " << std::hex << std::setw(8) << m_pDevice->GetPhysicalDeviceProperties().deviceID << "\n";
            }
            if (memcmp(pipelineCacheUUID, m_pDevice->GetPhysicalDeviceProperties().pipelineCacheUUID, sizeof(pipelineCacheUUID)) != 0)
            {
                badCache = true;
                std::cout << "  UUID mismatch in " << path << ".\n";
                std::cout << "    Cache contains: " << core::UUID(pipelineCacheUUID) << "\n";
                std::cout << "    Driver expects: " << core::UUID(m_pDevice->GetPhysicalDeviceProperties().pipelineCacheUUID) << "\n";
            }
            if (badCache)
            {
                // Don't submit initial cache data if any version info is incorrect
                free(startCacheData);
                startCacheSize = 0;
                startCacheData = nullptr;
                // And clear out the old cache file for use in next run
                std::cout << "  Deleting cache entry " << path << " to repopulate.\n";
                if (remove(path.c_str()) != 0)
                {
                    std::cerr << "Reading error";
                    exit(EXIT_FAILURE);
                }
            }
        }
        // Feed the initial cache data into cache creation
        vk::PipelineCacheCreateInfo cacheCreateInfo = {
            .initialDataSize = startCacheSize,
            .pInitialData = startCacheData,
        };
        vk::UniquePipelineCache pipelineCache = m_pDevice->GetVkDevice().createPipelineCacheUnique(cacheCreateInfo);
        // Free our initialData now that pipeline cache has been created
        free(startCacheData);
        startCacheData = NULL;

        return pipelineCache;
    }
}; // namespace vkg
} // namespace vkg