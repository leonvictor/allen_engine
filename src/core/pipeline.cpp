#pragma once

#include "../utils/files.cpp" // TODO: Berk
#include "../utils/uuid.cpp"  // TODO: Berk
#include "../vertex.hpp"      // TODO: Berk
#include "device.hpp"
#include "shaders.hpp"
#include <iomanip>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace core
{

struct Viewport : vk::Viewport
{

    Viewport()
    {
        init();
    }

    void init()
    {
        this->x = 0.0f;
        this->y = 0.0f;
        this->height = 0.0f;
        this->width = 0.0f;
        this->minDepth = 0.0f;
        this->maxDepth = 1.0f;
    }
};

struct Pipeline
{
    std::shared_ptr<core::Device> device;
    vk::UniquePipeline pipeline;
    vk::UniquePipelineLayout layout;

    Pipeline() {}
};

// An helper class for pipeline creation
// TODO: Pull out more parts of the pipelines parts to allow customization
class PipelineFactory
{
  public:
    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    vk::PipelineMultisampleStateCreateInfo multisample;

    core::Viewport viewport;
    vk::Rect2D scissor;

    explicit PipelineFactory(std::shared_ptr<core::Device> device)
    {
        this->device = device;
        init();
    }

    std::unique_ptr<core::Pipeline> create(std::vector<vk::DescriptorSetLayout> descriptorSetLayouts, std::string cachePath = "")
    {
        auto vertextAttributeDescriptions = Vertex::getAttributeDescription();
        auto vertexBindingDescription = Vertex::getBindingDescription();

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertextAttributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = vertextAttributeDescriptions.data();
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{{},
                                                                   vk::PrimitiveTopology::eTriangleList,
                                                                   VK_FALSE};

        // TODO: Handle multiple viewports/scissors
        vk::PipelineViewportStateCreateInfo viewportStateInfo;
        viewportStateInfo.scissorCount = 1;
        viewportStateInfo.pScissors = &scissor;
        viewportStateInfo.viewportCount = 1;
        viewportStateInfo.pViewports = &viewport;

        // Color blending = How do we combine the color returned by the fragment shader and the one that is already in the pixel ?
        // Configuration per attached framebuffer
        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        // Alpha blending -> the new color is blended with the old one based on its opacity :
        // finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor
        // finalColor.a = newAlpha.a
        colorBlendAttachment.blendEnable = VK_FALSE; // Disabled for now
        colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;
        colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;

        // Configuration for global color blending settings.
        vk::PipelineColorBlendStateCreateInfo colorBlendInfo;
        colorBlendInfo.logicOpEnable = VK_FALSE;
        colorBlendInfo.attachmentCount = 1;
        colorBlendInfo.pAttachments = &colorBlendAttachment;
        // We can also define custom blend constants in colorBlendInfo.blendConstants[0, 1, 2, 3]

        // Not used for now
        // TODO: Pull out ?
        vk::PipelineLayoutCreateInfo layoutInfo;
        layoutInfo.setLayoutCount = descriptorSetLayouts.size(); // Update when we have more layouts
        layoutInfo.pSetLayouts = descriptorSetLayouts.data();

        layout = device->logical->createPipelineLayoutUnique(layoutInfo);

        // Shader stages
        pipelineCreateInfo.stageCount = shaderStages.size();
        pipelineCreateInfo.pStages = shaderStages.data();

        // Fixed-functions states
        pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendInfo;
        pipelineCreateInfo.pViewportState = &viewportStateInfo;
        // pipelineCreateInfo.pMultisampleState = &multisampleInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;

        vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo = {{},
                                                                     static_cast<uint32_t>(dynamicStates.size()),
                                                                     dynamicStates.data()};

        pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;

        // Pipeline layout
        pipelineCreateInfo.layout = layout.get();

        // Render pass
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = vk::Pipeline();

        // TODO: Generate different pipeline cache path depending on the options
        vk::UniquePipelineCache pipelineCache = loadCachedPipeline(device, cachePath);                                                // TODO
        vk::UniquePipeline graphicsPipeline = device->logical->createGraphicsPipelineUnique(pipelineCache.get(), pipelineCreateInfo); // TODO

        // TODO: Avoid saving if not needed
        // Store away the cache that we've populated.  This could conceivably happen
        // earlier, depends on when the pipeline cache stops being populated
        // internally.
        std::vector<uint8_t> endCacheData = device->logical->getPipelineCacheData(pipelineCache.get());

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

        clearShaders();
        dynamicStates.clear();

        std::unique_ptr<Pipeline> pipeline = std::make_unique<Pipeline>();
        pipeline->device = device;
        pipeline->layout = std::move(layout);
        pipeline->pipeline = std::move(graphicsPipeline);

        return std::move(pipeline);
    }

    // Set viewport and scissors to match a whole extent.
    void setExtent(const vk::Extent2D& extent)
    {
        viewport.width = extent.width;
        viewport.height = extent.height;
        scissor.extent = extent;
    }

    void setRenderPass(const vk::RenderPass& renderPass)
    {
        pipelineCreateInfo.renderPass = renderPass;
    }

    void registerShader(const std::string& filename, vk::ShaderStageFlagBits stage, std::string entrypoint = "main")
    {
        auto shaderStage = core::shaders::loadShader(device, filename, stage, entrypoint);
        shaderStages.push_back(shaderStage);
    }

    void clearShaders()
    {
        // TODO: Let the Shaders auto-destroy when they go out of scope
        for (const auto& shader : shaderStages)
        {
            device->logical->destroyShaderModule(shader.module);
        }
        shaderStages.clear();
    }

    void addDynamicState(vk::DynamicState state)
    {
        dynamicStates.push_back(state);
    }

  private:
    std::shared_ptr<core::Device> device;
    vk::UniquePipelineLayout layout;
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    // vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    std::vector<vk::DynamicState> dynamicStates;

    void init()
    {
        // TODO: set this at creation time ?
        pipelineCreateInfo.pDepthStencilState = &depthStencil;
        pipelineCreateInfo.pRasterizationState = &rasterizer;
        pipelineCreateInfo.pMultisampleState = &multisample;

        multisample.sampleShadingEnable = VK_TRUE;
        multisample.minSampleShading = .2f;
        multisample.rasterizationSamples = vk::SampleCountFlagBits(device->msaaSamples);

        scissor.offset = vk::Offset2D(0, 0);
        scissor.extent = vk::Extent2D(0, 0);

        // Initialize some parts with default values. We can modify them before calling create()
        // in order to specify some options.
        // TODO: Move that out. Default values belong in a constructor ?
        // Dynamic state: some of the states can be modified without recreating a pipeline, but will require arguments to be provided at draw time.
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = vk::CompareOp::eLess; // Lower depth = closer
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE; // Not used

        // Rasterizer turns the geometry shaped by the vertices into fragments to be colored by the fragment shader.
        rasterizer.depthClampEnable = VK_FALSE;        // Whether to clamp rather than discard vertices that are beyond the near and far planes.
        rasterizer.rasterizerDiscardEnable = VK_FALSE; // Disable any output to the framebuffer
        rasterizer.polygonMode = vk::PolygonMode::eFill;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = vk::CullModeFlagBits::eBack; // Type of face culling
        rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
        // The rasterizer can alter depth values.
        rasterizer.depthBiasEnable = VK_FALSE;
    }

    // Try to load a cached pipeline file. Based on https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/PipelineCache/PipelineCache.cpp
    // TODO:
    // - move to pipeline namespace
    // - remove verbose output OR make it debug only
    vk::UniquePipelineCache loadCachedPipeline(std::shared_ptr<core::Device> device, std::string path)
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
            if (vendorID != device->properties.vendorID)
            {
                badCache = true;
                std::cout << "  Vender ID mismatch in " << path << ".\n";
                std::cout << "    Cache contains: " << std::hex << std::setw(8) << vendorID << "\n";
                std::cout << "    Driver expects: " << std::hex << std::setw(8) << device->properties.vendorID << "\n";
            }
            if (deviceID != device->properties.deviceID)
            {
                badCache = true;
                std::cout << "  Device ID mismatch in " << path << ".\n";
                std::cout << "    Cache contains: " << std::hex << std::setw(8) << deviceID << "\n";
                std::cout << "    Driver expects: " << std::hex << std::setw(8) << device->properties.deviceID << "\n";
            }
            if (memcmp(pipelineCacheUUID, device->properties.pipelineCacheUUID, sizeof(pipelineCacheUUID)) != 0)
            {
                badCache = true;
                std::cout << "  UUID mismatch in " << path << ".\n";
                std::cout << "    Cache contains: " << core::UUID(pipelineCacheUUID) << "\n";
                std::cout << "    Driver expects: " << core::UUID(device->properties.pipelineCacheUUID) << "\n";
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
        vk::UniquePipelineCache pipelineCache = device->logical->createPipelineCacheUnique(
            vk::PipelineCacheCreateInfo(vk::PipelineCacheCreateFlags(), startCacheSize, startCacheData));
        // Free our initialData now that pipeline cache has been created
        free(startCacheData);
        startCacheData = NULL;

        return pipelineCache;
    }
};
} // namespace core