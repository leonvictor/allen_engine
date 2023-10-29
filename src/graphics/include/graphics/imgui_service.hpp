#pragma once


#include "instance.hpp"
#include "render_engine.hpp"
#include "render_pass.hpp"
#include "window.hpp"

#include <config/path.h>
#include <common/services/service.hpp>

#include <GLFW/glfw3.h>
#include <IconsFontAwesome6.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <imnodes.h>
#include <tracy/Tracy.hpp>

namespace aln
{

class ImGUIService : public IService
{
  public:
    void Initialize()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImNodes::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.05f, 0.05f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.04f, 0.05f, 0.05f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.04f, 0.05f, 0.05f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.05f, 0.07f, 0.08f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(1.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.05f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.04f, 0.05f, 0.06f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.04f, 0.05f, 0.06f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.05f, 0.07f, 0.07f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.21f, 0.31f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.37f, 0.61f, 1.00f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.10f, 0.13f, 0.15f, 1.00f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.56f, 1.00f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.05f, 0.07f, 0.08f, 0.55f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.05f, 0.07f, 0.08f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.04f, 0.05f, 0.05f, 1.00f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.05f, 0.07f, 0.08f, 1.00f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
        colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
        colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
        colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
        colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }

    void InitializeRendering(RenderEngine* pRenderEngine, RenderPass& renderPass)
    {
        assert(pRenderEngine != nullptr && renderPass);

        auto pGlfwWindow = reinterpret_cast<GLFWwindow*>(pRenderEngine->GetWindow()->GetWindowPtr());
        ImGui_ImplGlfw_InitForVulkan(pGlfwWindow, true);

        ImGui_ImplVulkan_InitInfo init_info;
        init_info.Instance = (VkInstance) pRenderEngine->GetInstance()->GetVkInstance();
        init_info.PhysicalDevice = pRenderEngine->GetVkPhysicalDevice();
        init_info.Device = pRenderEngine->GetVkDevice();
        init_info.QueueFamily = pRenderEngine->GetPresentQueue().GetFamilyIndex();
        init_info.Queue = pRenderEngine->GetPresentQueue().GetVkQueue();
        init_info.PipelineCache = nullptr;
        init_info.DescriptorPool = pRenderEngine->GetDescriptorPool();
        init_info.Allocator = nullptr;
        init_info.MinImageCount = 2;
        init_info.ImageCount = pRenderEngine->GetFrameQueueSize();
        init_info.CheckVkResultFn = nullptr;
        init_info.MSAASamples = (VkSampleCountFlagBits) pRenderEngine->GetMSAASamples();
        init_info.Subpass = 0;
        init_info.UseDynamicRendering = false;

        ImGui_ImplVulkan_Init(&init_info, renderPass.GetVkRenderPass());

        // Upload Fonts
        auto& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();

        // merge in icons from Font Awesome
        static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.GlyphOffset.y = 1;
        // icons_config.PixelSnapH = true;
        io.Fonts->AddFontFromFileTTF(FONTS_DIR "/fa-regular-400.ttf", 13.0f, &icons_config, icons_ranges);
        io.Fonts->AddFontFromFileTTF(FONTS_DIR "/fa-solid-900.ttf", 13.0f, &icons_config, icons_ranges);

        // Use any command queue
        pRenderEngine->GetGraphicsPersistentCommandPool().Execute([&](vk::CommandBuffer cb)
            { ImGui_ImplVulkan_CreateFontsTexture(cb); });

        pRenderEngine->GetGraphicsQueue().WaitIdle(); // Not ideal !
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    void ShutdownRendering()
    {
        ImGui_ImplVulkan_Shutdown();
    }

    void Shutdown()
    {
        // Cleanup ImGui
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        ImNodes::DestroyContext();
    }

    void StartFrame()
    {
        ZoneScoped;
 
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void EndFrame()
    {
        ZoneScoped;

        ImGui::Render();
    }

    void Render(TransientCommandBuffer& cb)
    {
        ImGui_ImplVulkan_NewFrame();
        ImDrawData* draw_data = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(draw_data, (vk::CommandBuffer) cb);
    }
};
} // namespace aln