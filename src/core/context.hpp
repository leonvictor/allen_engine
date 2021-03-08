#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "buffer.hpp"
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
    vk::UniqueSurfaceKHR surface;
    std::shared_ptr<core::Device> device;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation",
    };

    bool enableValidationLayers = true; // TODO: Move that somewhere else (global config)

    explicit Context(GLFWwindow* window);

  private:
    void createInstance();
    void createSurface(GLFWwindow* window);

    // Support and versions queries
    bool checkValidationLayersSupport();
    std::vector<const char*> getRequiredExtensions();
};
}; // namespace core