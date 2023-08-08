#pragma once

#include <imgui.h>
#include <imgui_internal.h>

/// Custom widgets
namespace aln::ImGuiWidgets
{
/// @brief Moveable splitter. https://github.com/ocornut/imgui/issues/319
static bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
{
    using namespace ImGui;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID("##Splitter");
    ImRect bb;
    auto minOffset = (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
    bb.Min = {window->DC.CursorPos.x + minOffset.x, window->DC.CursorPos.y + minOffset.y};
    auto offset = ImGui::CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
    bb.Max = {bb.Min.x + offset.x, bb.Min.y + offset.y};
    return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

static bool SplitterH(float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
{
    return Splitter(false, thickness, size1, size2, min_size1, min_size2, splitter_long_axis_size);
}

static bool SplitterV(float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
{
    return Splitter(true, thickness, size1, size2, min_size1, min_size2, splitter_long_axis_size);
}
} // namespace aln::ImGuiWidgets