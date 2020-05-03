#pragma once 

#include <vulkan/vulkan.hpp>
#include <memory>
#include <iomanip>
#include "device.hpp"
#include "../vertex.hpp" // TODO: Berk
#include "../utils/files.cpp" // TODO: Berk
#include "../utils/uuid.cpp"

namespace core {
    class Pipeline {
    public:
        vk::Pipeline graphicsPipeline; // Should be family-agnostic ? (i.e rename to "pipeline")
        vk::PipelineLayout layout;

        const std::string PIPELINE_CACHE_PATH = "pipeline_cache_data.bin";

        Pipeline() {}

        Pipeline(std::shared_ptr<core::Device> device, vk::Extent2D extent, std::vector<vk::DescriptorSetLayout> descriptorSetLayouts, vk::RenderPass renderPass) {
            this->device = device;
            createGraphicsPipeline(device, extent, descriptorSetLayouts, renderPass);
        }

        void destroy() {
            device->logicalDevice.destroyPipeline(graphicsPipeline);
            device->logicalDevice.destroyPipelineLayout(layout);
        }

        /* 
        * TODO: Move shader files locations out
        * */
        void createGraphicsPipeline(std::shared_ptr<core::Device> device, vk::Extent2D extent, std::vector<vk::DescriptorSetLayout> descriptorSetLayouts, vk::RenderPass renderPass) {
            this->device = device;

            auto vertShaderCode = utils::readFile("shaders/vert.spv");
            auto fragShaderCode = utils::readFile("shaders/frag.spv");

            auto vertShaderModule = createShaderModule(vertShaderCode);
            auto fragShaderModule = createShaderModule(fragShaderCode);

            vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
            vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex; // At which stage of the pipeline is this shader used 
            vertShaderStageInfo.module = vertShaderModule;
            vertShaderStageInfo.pName = "main"; // Which shader function to invoke (entrypoint)
            // pSpecializationInfo : We can set values for constants in the shader.
            // Then we can use a single shader module and have its behavior configured at pipeline creation (here)

            vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
            fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment; 
            fragShaderStageInfo.module = fragShaderModule;
            fragShaderStageInfo.pName = "main";

            vk::PipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo}; // Referenced in the actual pipeline creation step

            // Describe the format of the vertex data that will be passed to the vertex shader.
            auto vertextAttributeDescriptions = Vertex::getAttributeDescription();
            auto vertexBindingDescription = Vertex::getBindingDescription();

            vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
            vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertextAttributeDescriptions.size());;
            vertexInputInfo.pVertexAttributeDescriptions = vertextAttributeDescriptions.data();
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;

            // Describes :  what kind of geometry will be drawn from the vertices and if primitive restart should be nabled
            vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
            inputAssemblyInfo.topology = vk::PrimitiveTopology::eTriangleList;
            inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

            vk::Viewport viewport;
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.height = (float) extent.height;
            viewport.width = (float) extent.width;
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
            rasterizerInfo.depthClampEnable = VK_FALSE; // Whether to clamp rather than discard vertices that are beyond the near and far planes.
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

            // Dynamic state: some of the states can be modified without recreating a pipeline, but will require arguments to be provided at draw time.
            vk::PipelineDepthStencilStateCreateInfo depthStencil;
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = vk::CompareOp::eLess; // Lower depth = closer
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.minDepthBounds = 0.0f;
            depthStencil.maxDepthBounds = 1.0f;
            depthStencil.stencilTestEnable = VK_FALSE; // Not used

            // Not used for now
            vk::PipelineLayoutCreateInfo layoutInfo ;
            layoutInfo.setLayoutCount = descriptorSetLayouts.size(); // Update when we have more layouts
            layoutInfo.pSetLayouts = descriptorSetLayouts.data();

            this->layout = device->logicalDevice.createPipelineLayout(layoutInfo);

            // Finally create the graphics pipeline !
            vk::GraphicsPipelineCreateInfo pipelineInfo;
            
            // Shader stages
            pipelineInfo.stageCount = 2;
            pipelineInfo.pStages = shaderStages; 
            
            // Fixed-functions states
            pipelineInfo.pVertexInputState = &vertexInputInfo;
            pipelineInfo.pColorBlendState = &colorBlendInfo;
            pipelineInfo.pViewportState = &viewportStateInfo;
            pipelineInfo.pRasterizationState = &rasterizerInfo;
            pipelineInfo.pMultisampleState = &multisampleInfo;
            pipelineInfo.pDepthStencilState = &depthStencil;
            pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
            pipelineInfo.pDynamicState = nullptr;

            // Pipeline layout
            pipelineInfo.layout = this->layout;

            // Render pass
            pipelineInfo.renderPass = renderPass;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = vk::Pipeline();

            auto pipelineCache = loadCachedPipeline();
            graphicsPipeline = device->logicalDevice.createGraphicsPipeline(pipelineCache.get(), pipelineInfo); // TODO

            device->logicalDevice.destroyShaderModule(vertShaderModule);
            device->logicalDevice.destroyShaderModule(fragShaderModule);

            // Save the cache data 
            // TODO: We could store that in the class and save it on destroy 

            // Store away the cache that we've populated.  This could conceivably happen
            // earlier, depends on when the pipeline cache stops being populated
            // internally.
            std::vector<uint8_t> endCacheData = device->logicalDevice.getPipelineCacheData(pipelineCache.get());

            // Write the file to disk, overwriting whatever was there
            std::ofstream writeCacheStream(PIPELINE_CACHE_PATH, std::ios_base::out | std::ios_base::binary);
            if (writeCacheStream.good()) {
                writeCacheStream.write( reinterpret_cast<char const *>(endCacheData.data()), endCacheData.size());
                writeCacheStream.close();
                std::cout << "  cacheData written to " << PIPELINE_CACHE_PATH << "\n";
            }
            else {
                // Something bad happened
                std::cout << "  Unable to write cache data to disk!\n";
            }
        }

    private:
        std::shared_ptr<core::Device> device;

        vk::ShaderModule createShaderModule(const std::vector<char>& code) {
            vk::ShaderModuleCreateInfo createInfo;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            return device->logicalDevice.createShaderModule(createInfo);
        }

        /* Tries to load a cached pipeline file. Based on https://github.com/KhronosGroup/Vulkan-Hpp/blob/master/samples/PipelineCache/PipelineCache.cpp */ 
        vk::UniquePipelineCache loadCachedPipeline() {
            // Check disk for existing cache data
            size_t startCacheSize = 0;
            char * startCacheData = nullptr;

            std::ifstream readCacheStream(PIPELINE_CACHE_PATH, std::ios_base::in | std::ios_base::binary);
            if ( readCacheStream.good()) {
                // Determine cache size
                readCacheStream.seekg( 0, readCacheStream.end );
                startCacheSize = static_cast<size_t>( readCacheStream.tellg() );
                readCacheStream.seekg( 0, readCacheStream.beg );

                // Allocate memory to hold the initial cache data
                startCacheData = (char *)std::malloc( startCacheSize );

                // Read the data into our buffer
                readCacheStream.read( startCacheData, startCacheSize );

                // Clean up and print results
                readCacheStream.close();
                std::cout << "  Pipeline cache HIT!\n";
                std::cout << "  cacheData loaded from " << PIPELINE_CACHE_PATH << "\n";
            } else {
                // No cache found on disk
                std::cout << "  Pipeline cache miss!\n";
            }

            if (startCacheData) { 
                
                uint32_t headerLength = 0;
                uint32_t cacheHeaderVersion = 0;
                uint32_t vendorID = 0;
                uint32_t deviceID = 0;
                uint8_t  pipelineCacheUUID[VK_UUID_SIZE] = {};

                memcpy( &headerLength, (uint8_t *) startCacheData + 0, 4 );
                memcpy( &cacheHeaderVersion, (uint8_t *) startCacheData + 4, 4 );
                memcpy( &vendorID, (uint8_t *) startCacheData + 8, 4 );
                memcpy( &deviceID, (uint8_t *) startCacheData + 12, 4 );
                memcpy( pipelineCacheUUID, (uint8_t *) startCacheData + 16, VK_UUID_SIZE );

                // Check each field and report bad values before freeing existing cache
                bool badCache = false;
                if ( headerLength <= 0 ) {
                    badCache = true;
                    std::cout << "  Bad header length in " << PIPELINE_CACHE_PATH << ".\n";
                    std::cout << "    Cache contains: " << std::hex << std::setw( 8 ) << headerLength << "\n";
                }
                if ( cacheHeaderVersion != VK_PIPELINE_CACHE_HEADER_VERSION_ONE ) {
                    badCache = true;
                    std::cout << "  Unsupported cache header version in " << PIPELINE_CACHE_PATH << ".\n";
                    std::cout << "    Cache contains: " << std::hex << std::setw( 8 ) << cacheHeaderVersion << "\n";
                }
                if ( vendorID != device->properties.vendorID ) {
                    badCache = true;
                    std::cout << "  Vender ID mismatch in " << PIPELINE_CACHE_PATH << ".\n";
                    std::cout << "    Cache contains: " << std::hex << std::setw( 8 ) << vendorID << "\n";
                    std::cout << "    Driver expects: " << std::hex << std::setw( 8 ) << device->properties.vendorID << "\n";
                }
                if ( deviceID != device->properties.deviceID ) {
                    badCache = true;
                    std::cout << "  Device ID mismatch in " << PIPELINE_CACHE_PATH << ".\n";
                    std::cout << "    Cache contains: " << std::hex << std::setw( 8 ) << deviceID << "\n";
                    std::cout << "    Driver expects: " << std::hex << std::setw( 8 ) << device->properties.deviceID << "\n";
                }
                if ( memcmp( pipelineCacheUUID, device->properties.pipelineCacheUUID, sizeof( pipelineCacheUUID ) ) != 0 ) {
                    badCache = true;
                    std::cout << "  UUID mismatch in " << PIPELINE_CACHE_PATH << ".\n";
                    std::cout << "    Cache contains: " << core::UUID( pipelineCacheUUID ) << "\n";
                    std::cout << "    Driver expects: " << core::UUID( device->properties.pipelineCacheUUID ) << "\n";
                }
                if ( badCache ) {
                    // Don't submit initial cache data if any version info is incorrect
                    free( startCacheData );
                    startCacheSize = 0;
                    startCacheData = nullptr;
                    // And clear out the old cache file for use in next run
                    std::cout << "  Deleting cache entry " << PIPELINE_CACHE_PATH << " to repopulate.\n";
                    if ( remove( PIPELINE_CACHE_PATH.c_str() ) != 0 ) {
                        std::cerr << "Reading error";
                        exit( EXIT_FAILURE );
                    }
                }
            }
            // Feed the initial cache data into cache creation
            vk::UniquePipelineCache pipelineCache = device->logicalDevice.createPipelineCacheUnique(
                vk::PipelineCacheCreateInfo(vk::PipelineCacheCreateFlags(), startCacheSize, startCacheData));
            // Free our initialData now that pipeline cache has been created
            free( startCacheData );
            startCacheData = NULL;

            return pipelineCache;
        }
    };
}