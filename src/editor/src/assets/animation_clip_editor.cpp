#include "assets/animation_clip_editor.hpp"

#include <IconsFontAwesome6.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

namespace aln
{

static bool SliderTimeline(ImGuiID id, float* pCurrentTime, const float animDuration)
{
    ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
    if (pWindow->SkipItems)
    {
        return false;
    }

    auto pContext = ImGui::GetCurrentContext();
    const ImGuiStyle& style = pContext->Style;

    // TMP: see https://github.com/ocornut/imgui/issues/2486
    auto contentRegionMin = ImGui::GetWindowContentRegionMin();
    auto contentRegionMax = ImGui::GetWindowContentRegionMax();
    {
        auto windowPos = ImGui::GetWindowPos();
        contentRegionMin.x += windowPos.x;
        contentRegionMin.y += windowPos.y;
        contentRegionMax.x += windowPos.x;
        contentRegionMax.y += windowPos.y;
    }

    const auto timelinePosY = ImGui::GetCursorPosY();

    const auto timelineWidth = contentRegionMax.x - contentRegionMin.x;
    const auto timelineHeight = 40;
    const ImRect timelineBB = {{contentRegionMin.x, contentRegionMin.y},
        {contentRegionMin.x + timelineWidth, contentRegionMin.y + timelineHeight}};

    ImGui::ItemSize(timelineBB);
    if (!ImGui::ItemAdd(timelineBB, id, &timelineBB, ImGuiItemAddFlags_None))
    {
        return false;
    }

    const bool hovered = ImGui::ItemHoverable(timelineBB, id);

    // TODO: Not focusable ?
    const bool focusRequested = (pWindow->DC.LastItemStatusFlags & ImGuiItemStatusFlags_Focused) != 0;
    const bool clicked = (hovered && pContext->IO.MouseClicked[0]);
    if (focusRequested || clicked || pContext->NavActivateId == id || pContext->NavInputId == id)
    {
        ImGui::SetActiveID(id, pWindow);
        ImGui::SetFocusID(id, pWindow);
        ImGui::FocusWindow(pWindow);
        pContext->ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
    }

    // Draw timeline
    auto pDrawList = ImGui::GetWindowDrawList();
    const auto majorGraduationOffset = timelineWidth / animDuration;
    const auto minorGraduationOffset = majorGraduationOffset / 10;

    for (auto time = 0; time < animDuration; time += 1.0f)
    {
        // Major graduations
        const auto majorGraduationPosX = contentRegionMin.x + (time * majorGraduationOffset);
        const auto majorGraduationPosY = contentRegionMin.y;
        pDrawList->AddRectFilled({majorGraduationPosX - 1, majorGraduationPosY}, {majorGraduationPosX, majorGraduationPosY + timelineHeight}, IM_COL32_WHITE);
        pDrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), {majorGraduationPosX + 5.0f, majorGraduationPosY}, IM_COL32_WHITE, std::to_string(time).c_str());

        // Minor graduations
        for (auto ms = 1; ms < 10; ++ms)
        {
            const auto minorGraduationPosX = majorGraduationPosX + (minorGraduationOffset * ms);
            pDrawList->AddRectFilled({minorGraduationPosX - 1, majorGraduationPosY + timelineHeight / 2}, {minorGraduationPosX, majorGraduationPosY + timelineHeight}, IM_COL32_WHITE);
        }
    }

    // Slider behavior
    ImRect grabBB;
    const bool valueChanged = ImGui::SliderBehaviorT<float, float, float>(timelineBB, id, ImGuiDataType_Float, pCurrentTime, 0, animDuration, "", ImGuiSliderFlags_None, &grabBB);
    if (valueChanged)
    {
        ImGui::MarkItemEdited(id);
    }

    // Render cursor
    const auto cursorWidth = 10.0f;
    const auto cursorHeight = timelineHeight / 2;
    if (grabBB.Max.x > grabBB.Min.x)
    {
        const auto cursorPos = ImVec2(grabBB.Min.x + grabBB.GetWidth() / 2, contentRegionMin.y + timelineHeight / 2);
        const ImVec2 cursorPoints[] = {
            {cursorPos.x - cursorWidth / 2, cursorPos.y},
            {cursorPos.x + cursorWidth / 2, cursorPos.y},
            {cursorPos.x + cursorWidth / 2, cursorPos.y + cursorHeight / 2},
            {cursorPos.x, cursorPos.y + cursorHeight},
            {cursorPos.x - cursorWidth / 2, cursorPos.y + cursorHeight / 2}};

        pDrawList->AddConvexPolyFilled(cursorPoints, 5, IM_COL32_WHITE);
    }

    return valueChanged;
}

bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f)
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

void AnimationClipEditor::Update(const UpdateContext& context)
{
    float animDuration = 5; // TMP. Animation duration in seconds
    static float previewHeight = 300;
    static float trackHeight = 300;

    if (ImGui::Begin("Animation Clip", &m_isOpen))
    {
        float contentWidth = ImGui::GetContentRegionAvailWidth();
        Splitter(false, 2, &previewHeight, &trackHeight, 8, 8, contentWidth);
        // TODO: Allow re-docking subwindows but only inside the "main" one
        if (ImGui::BeginChild("Animation Preview", ImVec2{contentWidth, previewHeight}, true))
        {
            // TODO: Animation preview
            ImGui::Text("Animation Preview");
            ImGui::EndChild();
        }

        if (ImGui::BeginChild("Event Tracks", ImVec2(contentWidth, trackHeight), true))
        {
            /// --------- Header
            // Anim player control
            ImGui::Text(ICON_FA_BACKWARD_STEP);
            ImGui::SameLine();
            ImGui::Text(ICON_FA_PLAY);
            ImGui::SameLine();
            ImGui::Text(ICON_FA_FORWARD_STEP);
            ImGui::SameLine();
            ImGui::Text("%.2f / %.2f", m_animationTime, animDuration);

            static float listWidth = 50;
            static float timelineWidth = contentWidth - listWidth;
            const float trackEditorHeight = ImGui::GetWindowContentRegionMax().y - ImGui::GetWindowContentRegionMin().y - 2;
            Splitter(true, 2, &listWidth, &timelineWidth, 8, 8, trackEditorHeight);

            // --------- Event Track List
            ImGui::BeginChild("TrackList", ImVec2(listWidth, trackEditorHeight), true);

            ImGui::Dummy(ImVec2(0.0f, 40)); // TODO: Use timeline height

            size_t trackToEraseIdx = InvalidIndex;
            const auto eventTrackCount = m_eventTracks.size();
            for (auto trackIndex = 0; trackIndex < eventTrackCount; ++trackIndex)
            {
                ImGui::PushID(trackIndex);

                auto pEventTrack = m_eventTracks[trackIndex];

                ImGui::InputText("##TrackName", &pEventTrack->m_name);

                ImGui::SameLine();
                if (ImGui::SmallButton(ICON_FA_TRASH_CAN))
                {
                    trackToEraseIdx = trackIndex;
                }

                ImGui::PopID();
            }

            ImGui::Text("Add Track");
            ImGui::SameLine();
            if (ImGui::SmallButton(ICON_FA_PLUS))
            {
                auto pTrack = aln::New<EventTrack>();
                m_eventTracks.push_back(pTrack);
            }

            if (trackToEraseIdx != InvalidIndex)
            {
                aln::Delete(m_eventTracks[trackToEraseIdx]);
                m_eventTracks.erase(m_eventTracks.begin() + trackToEraseIdx);
            }

            ImGui::EndChild();

            ImGui::SameLine();

            ImGui::BeginChild("TimelineEditor", ImVec2(timelineWidth, trackEditorHeight), true);

            // --------- Timeline
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2());
            SliderTimeline(ImGui::GetID("test2"), &m_animationTime, animDuration);
            ImGui::PopStyleVar();

            // --------- Editor
            // TODO: Draw grid
            // TODO: Highlight selected
            // TODO: Draw cursor line

            ImGui::EndChild();

            ImGui::EndChild();
        }
    }
    ImGui::End();

    if (!m_isOpen)
    {
        RequestAssetWindowDeletion(GetID());
    }
}

void AnimationClipEditor::Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile)
{
}

void aln::AnimationClipEditor::Shutdown()
{
}

void aln::AnimationClipEditor::Clear()
{
}

AnimationClip* aln::AnimationClipEditor::Compile()
{
    return nullptr;
}
} // namespace aln