#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "device.hpp"
#include "instance.hpp"
#include "render_pass.hpp"
#include "window.hpp"

namespace vkg
{

/// @brief Handles RAII for ImGui.
class ImGUI
{
  public:
    ~ImGUI()
    {
        // Cleanup ImGui
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void Initialize(GLFWwindow* pGlfwWindow, std::shared_ptr<Device> pDevice, RenderPass& renderPass, int nSwapchainImages)
    {
        // Initialize Imgui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        const ImGuiIO& io = ImGui::GetIO();
        (void) io;

        // ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForVulkan(pGlfwWindow, true);

        ImGui_ImplVulkan_InitInfo init_info;
        init_info.Instance = (VkInstance) vkg::Instance::Get();
        init_info.PhysicalDevice = pDevice->GetVkPhysicalDevice();
        init_info.Device = pDevice->GetVkDevice();
        init_info.QueueFamily = pDevice->GetPresentQueue().GetFamilyIndex();
        init_info.Queue = pDevice->GetPresentQueue().GetVkQueue();
        init_info.PipelineCache = nullptr;
        init_info.DescriptorPool = pDevice->GetDescriptorPool();
        init_info.Allocator = nullptr;
        init_info.MinImageCount = 2;
        init_info.ImageCount = nSwapchainImages;
        init_info.CheckVkResultFn = nullptr;
        init_info.MSAASamples = (VkSampleCountFlagBits) pDevice->GetMSAASamples();
        init_info.Subpass = 0;

        ImGui_ImplVulkan_Init(&init_info, renderPass.GetVkRenderPass());

        // Upload Fonts
        // Use any command queue

        pDevice->GetGraphicsCommandPool().Execute([&](vk::CommandBuffer cb)
                                                  { ImGui_ImplVulkan_CreateFontsTexture(cb); });

        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void NewFrame()
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Render(vk::CommandBuffer& cb)
    {
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, cb);
    }
};
} // namespace vkg