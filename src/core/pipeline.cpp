#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
#include <iomanip>
#include "device.hpp"
#include "../vertex.hpp"      // TODO: Berk
#include "../utils/files.cpp" // TODO: Berk
#include "../utils/uuid.cpp"  // TODO: Berk
#include "shaders.cpp"

namespace core
{

const std::string PIPELINE_CACHE_PATH = "pipeline_cache_data.bin";

class Pipeline
{
public:
    vk::Pipeline graphicsPipeline; // Should be family-agnostic ? (i.e rename to "pipeline")
    vk::PipelineLayout layout;

    Pipeline() {}

    Pipeline(std::shared_ptr<core::Device> device,
             vk::Extent2D extent,
             std::vector<vk::DescriptorSetLayout> descriptorSetLayouts,
             vk::RenderPass renderPass,
             std::string vertexShaderFileName,
             std::string fragmentShaderFileName)
    {

        this->device = device;
        // this->vertexShaderFileName = vertexShaderFileName;
        // this->fragmentShaderFileName = fragmentShaderFileName;

        // createGraphicsPipeline(device, extent, descriptorSetLayouts, renderPass);
    }

    void destroy()
    {
        // device->logicalDevice.destroyPipeline(graphicsPipeline);
        // device->logicalDevice.destroyPipelineLayout(layout);
    }

private:
    std::shared_ptr<core::Device> device;
};

/* An helper class for pipeline creation
 * TODO: Pull out more parts of the pipelines parts to allow customization
*/
class PipelineFactory
{
public:
    vk::PipelineDepthStencilStateCreateInfo depthStencil;

    PipelineFactory(std::shared_ptr<core::Device> device, const vk::RenderPass &renderpass)
    {
        this->device = device;
        // this->layout = layout;
        this->renderpass = renderpass;
        init();
    }

    core::Pipeline create(vk::Extent2D &extent, std::vector<vk::DescriptorSetLayout> descriptorSetLayouts)
    {
        auto vertextAttributeDescriptions = Vertex::getAttributeDescription();
        auto vertexBindingDescription = Vertex::getBindingDescription();

        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertextAttributeDescriptions.size());
        ;
        vertexInputInfo.pVertexAttributeDescriptions = vertextAttributeDescriptions.data();
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;

        vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo{{},
                                                                   vk::PrimitiveTopology::eTriangleList,
                                                                   VK_FALSE};

        vk::Viewport viewport;
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.height = (float)extent.height;
        viewport.width = (float)extent.width;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor;
        scissor.offset = vk::Offset2D(0, 0);
        scissor.extent = extent;

        vk::PipelineViewportStateCreateInfo viewportStateInfo;
        viewportStateInfo.scissorCount = 1;
        viewportStateInfo.pScissors = &scissor;
        viewportStateInfo.viewportCount = 1;
        viewportStateInfo.pViewports = &viewport;

        // Rasterizer turns the geometry shaped by the vertices into fragments to be colored by the fragment shader.
        vk::PipelineRasterizationStateCreateInfo rasterizerInfo;
        rasterizerInfo.depthClampEnable = VK_FALSE;        // Whether to clamp rather than discard vertices that are beyond the near and far planes.
        rasterizerInfo.rasterizerDiscardEnable = VK_FALSE; // Disable any output to the framebuffer
        rasterizerInfo.polygonMode = vk::PolygonMode::eFill;
        rasterizerInfo.lineWidth = 1.0f;
        rasterizerInfo.cullMode = vk::CullModeFlagBits::eBack; // Type of face culling
        rasterizerInfo.frontFace = vk::FrontFace::eCounterClockwise;
        // The rasterizer can alter depth values.
        rasterizerInfo.depthBiasEnable = VK_FALSE;

        vk::PipelineMultisampleStateCreateInfo multisampleInfo;
        multisampleInfo.sampleShadingEnable = VK_TRUE;
        multisampleInfo.minSampleShading = .2f;
        multisampleInfo.rasterizationSamples = vk::SampleCountFlagBits(device->msaaSamples);

        // Depth and stencil testing. Required when using a depth or stencil buffer.
        // Not implemented for now.

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

        auto layout = device->logicalDevice.createPipelineLayout(layoutInfo);

        // Shader stages
        pipelineCreateInfo.stageCount = shaderStages.size();
        pipelineCreateInfo.pStages = shaderStages.data();

        // Fixed-functions states
        pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
        pipelineCreateInfo.pColorBlendState = &colorBlendInfo;
        pipelineCreateInfo.pViewportState = &viewportStateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizerInfo;
        pipelineCreateInfo.pMultisampleState = &multisampleInfo;
        pipelineCreateInfo.pDepthStencilState = &depthStencil;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyInfo;
        pipelineCreateInfo.pDynamicState = nullptr;

        // Pipeline layout
        pipelineCreateInfo.layout = layout;

        // Render pass
        pipelineCreateInfo.renderPass = renderpass;
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = vk::Pipeline();

        auto pipelineCache = loadCachedPipeline(device, PIPELINE_CACHE_PATH); // TODO
        auto graphicsPipeline = device->logicalDevice.createGraphicsPipeline(pipelineCache.get(), pipelineCreateInfo); // TODO
        
        Pipeline pipeline;
        pipeline.layout = layout;
        pipeline.graphicsPipeline = graphicsPipeline;
        
        clearShaders();

        return pipeline;
    }

    void registerShader(const std::string &filename, vk::ShaderStageFlagBits stage, std::string entrypoint = "main")
    {
        auto shaderStage = core::shaders::loadShader(device, filename, stage, entrypoint);
        shaderStages.push_back(shaderStage);
    }

    void clearShaders()
    {
        for (const auto &shader : shaderStages)
        {
            device->logicalDevice.destroyShaderModule(shader.module);
        }
        shaderStages.clear();
    }

private:
    std::shared_ptr<core::Device> device;
    vk::PipelineLayout layout;
    vk::RenderPass renderpass;
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

    void init() {
        pipelineCreateInfo.pDepthStencilState = &depthStencil;

        // Initialize some parts with default values. We can modify them before calling create()
        // in order to specify some options.
        // TODO: Move that out
        // Dynamic state: some of the states can be modified without recreating a pipeline, but will require arguments to be provided at draw time.
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = vk::CompareOp::eLess; // Lower depth = closer
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.minDepthBounds = 0.0f;
        depthStencil.maxDepthBounds = 1.0f;
        depthStencil.stencilTestEnable = VK_FALSE; // Not used
    }

    /* Try to load a cached pipeline file. Based on https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/PipelineCache/PipelineCache.cpp 
     * TODO:
     * - move to pipeline namespace
     * - remove verbose output OR make it debug only    
    */
    vk::UniquePipelineCache loadCachedPipeline(std::shared_ptr<core::Device> device, std::string path)
    {
        // Check disk for existing cache data
        size_t startCacheSize = 0;
        char *startCacheData = nullptr;
        std::ifstream readCacheStream(path, std::ios_base::in | std::ios_base::binary);
        if (readCacheStream.good())
        {
            // Determine cache size
            readCacheStream.seekg(0, readCacheStream.end);
            startCacheSize = static_cast<size_t>(readCacheStream.tellg());
            readCacheStream.seekg(0, readCacheStream.beg);

            // Allocate memory to hold the initial cache data
            startCacheData = (char *)std::malloc(startCacheSize);

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

            memcpy(&headerLength, (uint8_t *)startCacheData + 0, 4);
            memcpy(&cacheHeaderVersion, (uint8_t *)startCacheData + 4, 4);
            memcpy(&vendorID, (uint8_t *)startCacheData + 8, 4);
            memcpy(&deviceID, (uint8_t *)startCacheData + 12, 4);
            memcpy(pipelineCacheUUID, (uint8_t *)startCacheData + 16, VK_UUID_SIZE);

            // Check each field and report bad values before freeing existing cache
            bool badCache = false;
            if (headerLength <= 0)
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
        vk::UniquePipelineCache pipelineCache = device->logicalDevice.createPipelineCacheUnique(
            vk::PipelineCacheCreateInfo(vk::PipelineCacheCreateFlags(), startCacheSize, startCacheData));
        // Free our initialData now that pipeline cache has been created
        free(startCacheData);
        startCacheData = NULL;

        return pipelineCache;
    }
};
} // namespace core