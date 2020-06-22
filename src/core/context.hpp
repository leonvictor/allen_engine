#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "buffer.cpp"
#include "commandpool.cpp"
#include "device.hpp"

#include <iostream>
#include <memory>

namespace core
{
struct UniformBufferObject
{
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec3 cameraPos;
};

class Context
{
  public:
    vk::UniqueInstance instance;
    vk::UniqueSurfaceKHR surface;
    std::shared_ptr<core::Device> device;

    core::CommandPool graphicsCommandPool;
    core::CommandPool transferCommandPool;

    vk::UniqueHandle<vk::DebugUtilsMessengerEXT, vk::DispatchLoaderDynamic> debugMessenger;

    // const std::string TEXTURE_PATH = "assets/textures/camel.jpg"; // TODO: Nope ! Chuck testa

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    bool enableValidationLayers = true; // TODO: Move that somewhere else (global config)

    Context(GLFWwindow* window);
    void destroy();

    // Add a name to a vulkan object for debugging purposes.
    template <class T>
    void setDebugUtilsObjectName(T object, std::string name)
    {
        vk::DebugUtilsObjectNameInfoEXT debugName{object.objectType, (uint64_t)(typename T::CType) object, name.c_str()};
        device->logical.get().setDebugUtilsObjectNameEXT(debugName, vk::DispatchLoaderDynamic{instance.get(), vkGetInstanceProcAddr});
    }

  private:
    void createInstance();
    void createSurface(GLFWwindow* window);
    void createCommandPools();

    // Support and versions queries
    bool checkValidationLayersSupport();
    std::vector<const char*> getRequiredExtensions();

    // Debugging tools
    vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo();
    void setupDebugMessenger();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT messageType,
        const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)
    {
        if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning)
        {
            std::cout << "Warning: " << pCallbackData->messageIdNumber << ":" << pCallbackData->pMessageIdName << ":" << pCallbackData->pMessage << std::endl;
        }
        else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
        {
            std::cerr << "Error: " << pCallbackData->messageIdNumber << ":" << pCallbackData->pMessageIdName << ":" << pCallbackData->pMessage << std::endl;
        }
        return VK_FALSE;
    }
};
}; // namespace core