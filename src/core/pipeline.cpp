#pragma once 

#include <vulkan/vulkan.hpp>
#include <memory>
#include "device.hpp"
#include "../vertex.hpp" // TODO: Berk
#include "../utils/files.cpp" // TODO: Berk

namespace core {
    class Pipeline {
    public:
        vk::Pipeline graphicsPipeline; // TODO: Should be family-agnostic (i.e rename to "pipeline")
        vk::PipelineLayout layout;

        Pipeline() {}

        Pipeline(std::shared_ptr<core::Device> device, vk::Extent2D extent, vk::DescriptorSetLayout descriptorSetLayout, vk::RenderPass renderPass) {
            // TODO
            this->device = device;
            createGraphicsPipeline(device, extent, descriptorSetLayout, renderPass);
        }

        void cleanup() {
            
            device->logicalDevice.destroyPipeline(graphicsPipeline);
            device->logicalDevice.destroyPipelineLayout(layout);
        }
        // ~Pipeline() {
            // TODO
        // }

        /* WIP : Requires some other classes to be moved first 
        * TODO: Move graphics specific stuff out ?
        * TODO: Move shader files locations out
        * TODO: Where does pipeline fit ? We could remove most of the required parameters i think
        * */
        void createGraphicsPipeline(std::shared_ptr<core::Device> device, vk::Extent2D extent, vk::DescriptorSetLayout descriptorSetLayout, vk::RenderPass renderPass) {
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
            
            // Rasterizer truns the geometry shaped by the vertices into fragments to be colors by the fragment shader.
            vk::PipelineRasterizationStateCreateInfo rasterizerInfo;
            rasterizerInfo.depthClampEnable = VK_FALSE; // Whether to clamp rather than discard vertices that are beyond the near and far planes.
            rasterizerInfo.rasterizerDiscardEnable = VK_FALSE; // Disable any output to the framebuffer
            rasterizerInfo.polygonMode = vk::PolygonMode::eFill;
            rasterizerInfo.lineWidth = 1.0f;
            rasterizerInfo.cullMode = vk::CullModeFlagBits::eBack; // Type of face culling
            rasterizerInfo.frontFace = vk::FrontFace::eCounterClockwise;
            // The rasterizer can alter depth values.
            rasterizerInfo.depthBiasEnable = VK_FALSE;

            // Disabled for now
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
            layoutInfo.setLayoutCount = 1;
            layoutInfo.pSetLayouts = &descriptorSetLayout;

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
            // pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.basePipelineHandle = vk::Pipeline();

            graphicsPipeline = device->logicalDevice.createGraphicsPipeline(vk::PipelineCache(), pipelineInfo); // TODO

            device->logicalDevice.destroyShaderModule(vertShaderModule);
            device->logicalDevice.destroyShaderModule(fragShaderModule);
        }

    private:
        std::shared_ptr<core::Device> device;

        vk::ShaderModule createShaderModule(const std::vector<char>& code) {
            vk::ShaderModuleCreateInfo createInfo;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            return device->logicalDevice.createShaderModule(createInfo);
        }
    };
}