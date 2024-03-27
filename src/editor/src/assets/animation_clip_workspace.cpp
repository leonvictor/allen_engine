#include "assets/animation_clip_workspace.hpp"
#include "aln_imgui_widgets.hpp"

#include <common/containers/algo.hpp>
#include <common/maths/maths.hpp>
#include <core/components/animation_player_component.hpp>
#include <core/components/camera_component.hpp>
#include <core/entity_systems/animation_system.hpp>
#include <core/entity_systems/camera_controller_system.hpp>
#include <core/world_systems/world_rendering_system.hpp>

#include <IconsFontAwesome6.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

namespace aln
{

/// @note adapted from UE source
/// @note We make a distinction between the value range that is being displayed and the actual graduations that will be drawn, and might have a different resolution
static AnimationClipWorkspace::TimelineGraduationsMetrics GetTimelineGraduationsParameters(float availableWidth, const AnimationClipWorkspace::TimelineRange& viewRange, float minGraduationIntervalPixels, float desiredMajorGraduationPixels)
{
    AnimationClipWorkspace::TimelineGraduationsMetrics metrics;
    metrics.m_pixelsPerValue = availableWidth / (viewRange.m_end - viewRange.m_start);

    Vector<uint32_t> commonBases;
    // Divide the rounded frame rate by 2s, 3s or 5s recursively
    {
        constexpr uint32_t denominators[] = {2, 3, 5};
        uint32_t lowestBase = 700; // TODO ?
        while (true)
        {
            commonBases.push_back(lowestBase);

            if (lowestBase % 2 == 0)
            {
                lowestBase = lowestBase / 2;
            }
            else if (lowestBase % 3 == 0)
            {
                lowestBase = lowestBase / 3;
            }
            else if (lowestBase % 5 == 0)
            {
                lowestBase = lowestBase / 5;
            }
            else
            {
                uint32_t lowestResult = lowestBase;
                for (uint32_t denominator : denominators)
                {
                    uint32_t result = lowestBase / denominator;
                    if (result > 0 && result < lowestResult)
                    {
                        lowestResult = result;
                    }
                }

                if (lowestResult < lowestBase)
                {
                    lowestBase = lowestResult;
                }
                else
                {
                    break;
                }
            }
        }
    }

    Algo::Reverse(commonBases);

    const uint32_t scale = static_cast<uint32_t>(Maths::Ceil(desiredMajorGraduationPixels / metrics.m_pixelsPerValue));
    const uint32_t baseIndex = Maths::Min(Algo::LowerBoundIndex(commonBases, scale), static_cast<uint32_t>(commonBases.size() - 1));
    const uint32_t base = commonBases[baseIndex];

    metrics.m_majorGraduationInterval = Maths::Ceil(scale / static_cast<float>(base)) * base;
    metrics.m_pixelsPerMajorGraduation = metrics.m_pixelsPerValue * metrics.m_majorGraduationInterval;

    // Find the lowest number of divisions we can show that's larger than the minimum tick size
    for (uint32_t divIndex = 0; divIndex < baseIndex; ++divIndex)
    {
        if (base % commonBases[divIndex] == 0)
        {
            const uint32_t minorDivisions = metrics.m_majorGraduationInterval / commonBases[divIndex];
            if (metrics.m_majorGraduationInterval / minorDivisions * metrics.m_pixelsPerValue >= minGraduationIntervalPixels)
            {
                metrics.m_minorGraduationDivisions = minorDivisions;
                break;
            }
        }
    }

    metrics.m_firstMajorGraduation = Maths::Floor(viewRange.m_start / metrics.m_majorGraduationInterval) * metrics.m_majorGraduationInterval;
    metrics.m_lastMajorGraduation = Maths::Ceil(viewRange.m_end / metrics.m_majorGraduationInterval) * metrics.m_majorGraduationInterval;
    return metrics;
}

static void DrawTimelineGraduations(const ImRect& timelineDrawArea, const AnimationClipWorkspace::TimelineRange& viewRange, const AnimationClipWorkspace::TimelineGraduationsMetrics& metrics)
{
    auto pDrawList = ImGui::GetWindowDrawList();

    float majorGraduationPosX = timelineDrawArea.Min.x + (metrics.m_firstMajorGraduation - viewRange.m_start) * metrics.m_pixelsPerValue;
    for (int majorGraduation = metrics.m_firstMajorGraduation; majorGraduation < metrics.m_lastMajorGraduation; majorGraduation += metrics.m_majorGraduationInterval)
    {
        // Major graduations
        const auto majorGraduationPosY = timelineDrawArea.Min.y;
        pDrawList->AddLine({majorGraduationPosX, majorGraduationPosY}, {majorGraduationPosX, majorGraduationPosY + timelineDrawArea.GetHeight() + 1.0f}, IM_COL32_WHITE);
        pDrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), {majorGraduationPosX + 5.0f, majorGraduationPosY}, IM_COL32_WHITE, std::to_string(majorGraduation).c_str());

        // Minor graduations
        for (uint32_t minorGraduation = 1; minorGraduation < metrics.m_minorGraduationDivisions; ++minorGraduation)
        {
            const auto minorGraduationPosX = majorGraduationPosX + (minorGraduation * metrics.m_pixelsPerMajorGraduation / metrics.m_minorGraduationDivisions);
            pDrawList->AddLine({minorGraduationPosX, majorGraduationPosY + timelineDrawArea.GetHeight() / 2}, {minorGraduationPosX, majorGraduationPosY + timelineDrawArea.GetHeight()}, IM_COL32_WHITE);
        }

        majorGraduationPosX += metrics.m_pixelsPerMajorGraduation;
    }
}

/// @brief Draw vertical grid lines on major graduations
static void DrawVerticalGridLines(const ImRect& timelineDrawArea, const AnimationClipWorkspace::TimelineRange& viewRange, const AnimationClipWorkspace::TimelineGraduationsMetrics& metrics)
{
    auto pDrawList = ImGui::GetWindowDrawList();

    const float windowBottomPosY = ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMax().y;
    float majorGraduationPosX = timelineDrawArea.Min.x + (metrics.m_firstMajorGraduation - viewRange.m_start) * metrics.m_pixelsPerValue;
    for (int majorGraduation = metrics.m_firstMajorGraduation; majorGraduation < metrics.m_lastMajorGraduation; majorGraduation += metrics.m_majorGraduationInterval)
    {
        const ImVec2 gridLineStart = {majorGraduationPosX, timelineDrawArea.Min.y};
        const ImVec2 gridLineEnd = {majorGraduationPosX, windowBottomPosY - 2.0f};
        pDrawList->AddLine(gridLineStart, gridLineEnd, ImGui::GetColorU32(ImGuiCol_Button));

        majorGraduationPosX += metrics.m_pixelsPerMajorGraduation;
    }
}

static void DrawTimelineMarker(const ImRect& timelineDrawArea, const AnimationClipWorkspace::TimelineRange& viewRange, float markerValue, const RGBColor& color)
{
    ImVec2 markerPosition = {
        Maths::Remap(viewRange.m_start, viewRange.m_end, timelineDrawArea.Min.x, timelineDrawArea.Max.x, markerValue),
        timelineDrawArea.Min.y,
    };

    const float markerWidth = 11.0f;
    const float markerHalfWidth = (markerWidth - 1) / 2;
    const float pointyEndHeight = 5.0f;

    const ImVec2 markerPoints[] = {
        {markerPosition.x - markerHalfWidth, markerPosition.y},                          // Top left
        {markerPosition.x + markerHalfWidth, markerPosition.y},                          // Top right
        {markerPosition.x + markerHalfWidth, timelineDrawArea.Max.y - pointyEndHeight},  // Bottom right
        {markerPosition.x, timelineDrawArea.Max.y},                                      // Pointy end
        {markerPosition.x - markerHalfWidth, timelineDrawArea.Max.y - pointyEndHeight}}; // Bottom left

    ImGui::GetWindowDrawList()->AddConvexPolyFilled(markerPoints, 5, (uint32_t) color);

    auto windowBottom = ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMax().y;
    auto lineEnd = ImVec2(markerPosition.x, windowBottom - 2.0f);
    ImGui::GetWindowDrawList()->AddLine(markerPosition, lineEnd, (uint32_t) color);
}

static bool SliderTimeline(ImGuiID id, float* pValue, const float minViewValue, const float maxViewValue, float minActualValue, float maxActualValue, ImRect* pOutSliderDrawingArea, ImVec2* pOutmarkerPosition)
{
    ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
    if (pWindow->SkipItems)
    {
        return false;
    }

    auto pContext = ImGui::GetCurrentContext();

    const float timelineHeight = 20;
    const ImVec2 timelineTopLeft = ImGui::GetWindowPos() + ImGui::GetCursorPos();
    const ImVec2 timelineBottomRight = timelineTopLeft + ImVec2(ImGui::GetContentRegionAvail().x, timelineHeight);

    const ImRect timelineBoundingBox = {
        timelineTopLeft,
        timelineBottomRight,
    };

    ImGui::ItemSize(timelineBoundingBox);
    if (!ImGui::ItemAdd(timelineBoundingBox, id, &timelineBoundingBox))
    {
        return false;
    }

    const bool hovered = ImGui::ItemHoverable(timelineBoundingBox, id, ImGuiItemFlags_None);
    // TODO: Not focusable ?
    // TMP: Temporary fix for newer ImGui version. It wasnt functionnal in the first place !
    // const bool focusRequested = (pWindow->DC.LastItemStatusFlags & ImGuiItemStatusFlags_Focused) != 0;
    const bool focusRequested = false;
    const bool clicked = (hovered && pContext->IO.MouseClicked[0]);
    // if (focusRequested || clicked || pContext->NavActivateId == id || pContext->NavInputId == id)
    if (focusRequested || clicked || pContext->NavActivateId == id)
    {
        ImGui::SetActiveID(id, pWindow);
        ImGui::SetFocusID(id, pWindow);
        ImGui::FocusWindow(pWindow);
        pContext->ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
    }

    // Slider behavior
    ImRect cursorGrabBB;
    bool valueChanged = ImGui::SliderBehaviorT<float, float, float>(timelineBoundingBox, id, ImGuiDataType_Float, pValue, minViewValue, maxViewValue, "", ImGuiSliderFlags_None, &cursorGrabBB);
    if (valueChanged)
    {
        ImGui::MarkItemEdited(id);
    }

    *pValue = Maths::Clamp(*pValue, minActualValue, maxActualValue);
    auto pDrawList = ImGui::GetWindowDrawList();

    // --- Set output ptrs
    *pOutSliderDrawingArea = timelineBoundingBox;

    return valueChanged;
}

void AnimationClipWorkspace::DrawAnimationPlaybackController()
{
    ImGui::Text(ICON_FA_BACKWARD_STEP);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Step Backward");
    }
    if (ImGui::IsItemClicked())
    {
        // Step to previous frame mark
        const auto frameTime = m_pAnimationPlayerComponent->GetFrameTime();
        const auto frameIdx = Maths::Clamp(Maths::Ceil((float) frameTime) - 1, 0.0f, m_pAnimationClip->GetFrameCount() - 1.0f);
        m_pAnimationPlayerComponent->SetFrameTime(FrameTime(frameIdx, 0.0f));
    }

    ImGui::SameLine();
    if (m_pAnimationPlayerComponent->IsPaused())
    {
        ImGui::Text(ICON_FA_PLAY);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Play");
        }
        if (ImGui::IsItemClicked())
        {
            m_pAnimationPlayerComponent->SetPaused(false);
        }
    }
    else
    {
        ImGui::Text(ICON_FA_PAUSE);
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Pause");
        }
        if (ImGui::IsItemClicked())
        {
            m_pAnimationPlayerComponent->SetPaused(true);
        }
    }

    ImGui::SameLine();
    ImGui::Text(ICON_FA_FORWARD_STEP);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Step Forward");
    }
    if (ImGui::IsItemClicked())
    {
        // Step to next frame mark
        const auto frameTime = m_pAnimationPlayerComponent->GetFrameTime();
        const auto frameIdx = Maths::Clamp(Maths::Floor((float) frameTime) + 1, 0.0f, m_pAnimationClip->GetFrameCount() - 1.0f);
        m_pAnimationPlayerComponent->SetFrameTime(FrameTime(frameIdx, 0.0f));
    }

    ImGui::SameLine();
    ImGui::Text("%.2f/%.2fs", m_pAnimationPlayerComponent->GetPercentageThroughAnimation() * m_pAnimationClip->GetDuration(), m_pAnimationClip->GetDuration());
    ImGui::SameLine();
    ImGui::Text("- %i/%i", m_pAnimationPlayerComponent->GetFrameTime().GetFrameIndex(), m_pAnimationClip->GetFrameCount() - 1);
    ImGui::SameLine();
    ImGui::Text("(%.2f%%)", m_pAnimationPlayerComponent->GetPercentageThroughAnimation() * 100);
}

void AnimationClipWorkspace::DrawAnimationEventsEditor()
{
    ImVec2 timelineCursorPos;
    ImRect timelineDrawingArea;

    auto TimelineDeltaToAnimTime = [&](float timelineDelta) -> Seconds
    { return (m_pAnimationClip->GetDuration() / timelineDrawingArea.GetWidth()) * timelineDelta; };

    auto AnimTimeToTimelinePosition = [&](Seconds animTime) -> float
    { return Maths::Remap(m_viewRange.m_start, m_viewRange.m_end, timelineDrawingArea.Min.x, timelineDrawingArea.Max.x, animTime / m_pAnimationClip->GetFramesPerSecond()); };

    const auto sequencerHeaderBgColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
    const auto sequencerBodyBgColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Header));
    const auto sequencerHoveredTrackBgColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered));
    const auto sequencerGridLinesColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Button));

    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, sequencerHeaderBgColor);
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, sequencerHeaderBgColor);

    ImGui::BeginTable("EventTable", 2, ImGuiTableFlags_Resizable);

    ImGui::TableSetupColumn("Track Headers");
    ImGui::TableSetupColumn("Track Details");

    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
    {
        ImGui::TableNextColumn();
        {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg)));
            
            // TODO: Header : track filtering ?
        }

        // Timeline
        ImGui::TableNextColumn();
        {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, sequencerHeaderBgColor);
            
            // Hack: Ensure the draw commands in the timeline editor will always be clipped
            ImGui::Dummy({ImGui::GetContentRegionAvail().x + ImGui::GetStyle().CellPadding.x + 1.0f, 0.0f});

            // Handle slider behavior and update playback value
            float sliderValue = (float) m_pAnimationPlayerComponent->GetFrameTime();
            SliderTimeline(ImGui::GetID("Animation Event Timeline"), &sliderValue, m_viewRange.m_start, m_viewRange.m_end, m_playbackRange.m_start, m_playbackRange.m_end, &timelineDrawingArea, &timelineCursorPos);

            float frameIndex;
            Percentage percentageThroughFrame = Maths::Modf(sliderValue, frameIndex);
            m_pAnimationPlayerComponent->SetFrameTime(FrameTime(frameIndex, percentageThroughFrame));

            // Compute and draw graduations
            auto biggestGraduationTextSize = Maths::Max(
                ImGui::CalcTextSize(std::to_string(static_cast<int>(m_viewRange.m_end)).c_str()).x,
                ImGui::CalcTextSize(std::to_string(static_cast<int>(m_viewRange.m_start)).c_str()).x);
            auto minGraduationSize = biggestGraduationTextSize + 5.0f;
            auto desiredGraduationSize = biggestGraduationTextSize * 2.0f;

            auto graduationsMetrics = GetTimelineGraduationsParameters(timelineDrawingArea.GetWidth(), m_viewRange, minGraduationSize, desiredGraduationSize);

            DrawTimelineGraduations(timelineDrawingArea, m_viewRange, graduationsMetrics);
            DrawVerticalGridLines(timelineDrawingArea, m_viewRange, graduationsMetrics);
        }
    }

    size_t trackToEraseIdx = InvalidIndex;
    const auto eventTrackCount = m_eventTracks.size();
    for (auto trackIndex = 0; trackIndex < eventTrackCount; ++trackIndex)
    {
        ImGui::PushID(trackIndex);
        ImGui::TableNextRow();

        auto pEventTrack = m_eventTracks[trackIndex];

        // Track list
        ImGui::TableNextColumn();
        {
            ImGui::InputText("##TrackName", &pEventTrack->m_name);

            ImGui::SameLine();
            if (ImGui::SmallButton(ICON_FA_TRASH_CAN))
            {
                trackToEraseIdx = trackIndex;
            }

            // TODO: Add color picker for the event track
        }

        // Track details
        ImGui::TableNextColumn();
        {
            const auto sequencerRowBgColor = ImGui::TableGetHoveredRow() == (trackIndex + 1) ? sequencerHoveredTrackBgColor : sequencerBodyBgColor;
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, sequencerRowBgColor);

            constexpr float trackRowHeight = 20;
            ImVec2 columnCursorPos = ImGui::GetCursorScreenPos();

            // Draw horizontal grid lines
            const ImVec2 gridLineStart = {timelineDrawingArea.Min.x - 4.0f, columnCursorPos.y + trackRowHeight + 1};
            const ImVec2 gridLineEnd = {timelineDrawingArea.Max.x - 1.0f, columnCursorPos.y + trackRowHeight + 1};
            ImGui::GetWindowDrawList()->AddLine(gridLineStart, gridLineEnd, sequencerGridLinesColor);

            // Enable click interactions
            ImGui::SetNextItemAllowOverlap();
            ImGui::InvisibleButton("track details", {ImGui::GetContentRegionAvail().x, trackRowHeight});

            if (ImGui::BeginPopupContextItem())
            {
                // TODO: Prevent adding event at the same time
                if (ImGui::MenuItem("Add event"))
                {
                    auto timelineAnimTime = m_pAnimationPlayerComponent->GetPercentageThroughAnimation() * m_pAnimationClip->GetDuration();

                    auto pEvent = aln::New<EditorAnimationEvent>();
                    pEvent->m_startTime = timelineAnimTime;
                    pEvent->m_endTime = timelineAnimTime;

                    pEventTrack->m_events.push_back(pEvent);
                }

                ImGui::EndPopup();
            }

            uint32_t eventToEraseIdx = InvalidIndex;

            const auto eventCount = pEventTrack->m_events.size();
            for (auto eventIdx = 0; eventIdx < eventCount; ++eventIdx)
            {
                constexpr float eventIconWidth = 10.0f;

                auto pEvent = pEventTrack->m_events[eventIdx];
                ImGui::PushID(pEvent);

                // --- Draw Event Start Icon
                ImGui::PushID("Start Event");

                ImVec2 startEventPosition = {
                    AnimTimeToTimelinePosition(pEvent->m_startTime),
                    columnCursorPos.y + trackRowHeight * 0.5f,
                };

                ImGui::GetWindowDrawList()->AddQuadFilled(
                    startEventPosition + ImVec2(eventIconWidth * 0.5f, 0.0f),
                    startEventPosition + ImVec2(0.0f, eventIconWidth * 0.5f),
                    startEventPosition - ImVec2(eventIconWidth * 0.5f, 0.0f),
                    startEventPosition - ImVec2(0.0f, eventIconWidth * 0.5f),
                    static_cast<uint32_t>(RGBColor::Green));

                // Place an invisible button to allow context interactions
                ImGui::SetCursorScreenPos({startEventPosition.x - eventIconWidth * 0.5f, startEventPosition.y - eventIconWidth * 0.5f});
                ImGui::InvisibleButton("Event", {eventIconWidth, eventIconWidth});

                if (ImGui::IsItemActivated())
                {
                    m_state.m_initialStartTime = pEvent->m_startTime;
                    m_state.m_initialEndTime = pEvent->m_endTime;
                }

                if (ImGui::IsItemActive())
                {
                    auto delta = ImGui::GetIO().MouseDelta.x;

                    if (Maths::IsNearEqual(m_state.m_initialStartTime, m_state.m_initialEndTime, EditorAnimationEvent::MinDurationEpsilon))
                    {
                        if (ImGui::GetIO().KeyCtrl)
                        {
                            // Turn into a durable event
                            pEvent->m_endTime += TimelineDeltaToAnimTime(delta);
                        }
                        else
                        {
                            // Move as an immediate event
                            pEvent->m_startTime += TimelineDeltaToAnimTime(delta);
                            pEvent->m_endTime = pEvent->m_startTime;
                        }
                    }
                    else
                    {
                        pEvent->m_startTime += TimelineDeltaToAnimTime(delta);
                    }
                }

                if (ImGui::IsItemDeactivated())
                {
                    if (Maths::IsNearEqual(pEvent->m_startTime, pEvent->m_endTime, EditorAnimationEvent::MinDurationEpsilon))
                    {
                        pEvent->m_startTime = pEvent->m_endTime;
                    }
                }

                if (ImGui::BeginPopupContextItem())
                {
                    if (pEvent->IsDurable())
                    {
                        ImGui::Text("Start: %.2fs", pEvent->m_startTime);
                        ImGui::Text("End: %.2fs", pEvent->m_endTime);
                    }
                    else
                    {
                        ImGui::Text("Time: %.2fs", pEvent->m_startTime);
                    }
                    if (ImGui::MenuItem("Delete"))
                    {
                        if (pEvent->IsDurable())
                        {
                            pEvent->m_startTime = pEvent->m_endTime;
                        }
                        else
                        {
                            eventToEraseIdx = eventIdx;
                        }
                    }
                    ImGui::EndPopup();
                }

                ImGui::PopID(); // Start Event

                if (pEvent->IsDurable())
                {
                    // --- Draw Event End Icon
                    ImGui::PushID("End Event");

                    ImVec2 endEventPosition = {
                        AnimTimeToTimelinePosition(pEvent->m_endTime),
                        columnCursorPos.y + trackRowHeight * 0.5f,
                    };

                    ImGui::GetWindowDrawList()->AddQuadFilled(
                        endEventPosition + ImVec2(eventIconWidth * 0.5f, 0.0f),
                        endEventPosition + ImVec2(0.0f, eventIconWidth * 0.5f),
                        endEventPosition - ImVec2(eventIconWidth * 0.5f, 0.0f),
                        endEventPosition - ImVec2(0.0f, eventIconWidth * 0.5f),
                        static_cast<uint32_t>(RGBColor::Green));

                    ImGui::SetCursorScreenPos({endEventPosition.x - eventIconWidth * 0.5f, endEventPosition.y - eventIconWidth * 0.5f});
                    ImGui::InvisibleButton("Durable Event End", {eventIconWidth, eventIconWidth});

                    if (ImGui::IsItemActivated())
                    {
                    }

                    if (ImGui::IsItemActive())
                    {
                        auto delta = ImGui::GetIO().MouseDelta.x;
                        pEvent->m_endTime += TimelineDeltaToAnimTime(delta);
                    }

                    if (ImGui::IsItemDeactivated())
                    {
                        if (Maths::IsNearEqual(pEvent->m_startTime, pEvent->m_endTime, EditorAnimationEvent::MinDurationEpsilon))
                        {
                            pEvent->m_endTime = pEvent->m_startTime;
                        }
                    }

                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem("Delete"))
                        {
                            pEvent->m_endTime = pEvent->m_startTime;
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID(); // End Event

                    // --- Draw durable event bar

                    ImGui::PushID("Durable Event Bar");

                    ImVec2* pFirstEventPosition = nullptr;
                    ImVec2* pSecondEventPosition = nullptr;
                    if (pEvent->m_startTime < pEvent->m_endTime)
                    {
                        pFirstEventPosition = &startEventPosition;
                        pSecondEventPosition = &endEventPosition;
                    }
                    else
                    {
                        pFirstEventPosition = &endEventPosition;
                        pSecondEventPosition = &startEventPosition;
                    }

                    ImRect durableEventBarBB = {
                        *pFirstEventPosition + ImVec2(0, -2),
                        *pSecondEventPosition + ImVec2(0, 2),
                    };

                    ImGui::GetWindowDrawList()->AddRectFilled(durableEventBarBB.Min, durableEventBarBB.Max, static_cast<uint32_t>(RGBColor::Green));

                    ImGui::SetCursorScreenPos(durableEventBarBB.Min);
                    ImGui::InvisibleButton("Durable Event Bar", durableEventBarBB.GetSize());

                    if (ImGui::IsItemActivated())
                    {
                        m_state.m_initialStartTime = pEvent->m_startTime;
                        m_state.m_initialEndTime = pEvent->m_endTime;
                    }

                    if (ImGui::IsItemActive())
                    {
                        auto delta = ImGui::GetIO().MouseDelta.x;
                        auto deltaTime = TimelineDeltaToAnimTime(delta);

                        pEvent->m_startTime += deltaTime;
                        pEvent->m_endTime += deltaTime;

                        // Clamp ranges
                        auto minTime = Maths::Min(pEvent->m_startTime, pEvent->m_endTime);
                        if (minTime < 0.0f)
                        {
                            pEvent->m_startTime -= minTime;
                            pEvent->m_endTime -= minTime;
                        }

                        auto maxTime = Maths::Max(pEvent->m_startTime, pEvent->m_endTime);
                        if (maxTime > m_pAnimationClip->GetDuration())
                        {
                            auto diff = maxTime - m_pAnimationClip->GetDuration();
                            pEvent->m_startTime -= diff;
                            pEvent->m_endTime -= diff;
                        }
                    }

                    ImGui::PopID(); // Durable event bar
                }

                // Enforce anim duration boundaries
                pEvent->m_startTime = Maths::Clamp(pEvent->m_startTime, 0.0f, m_pAnimationClip->GetDuration());
                pEvent->m_endTime = Maths::Clamp(pEvent->m_endTime, 0.0f, m_pAnimationClip->GetDuration());

                ImGui::PopID(); // Event
            }

            if (eventToEraseIdx != InvalidIndex)
            {
                aln::Delete(pEventTrack->m_events[eventToEraseIdx]);
                pEventTrack->m_events.erase(pEventTrack->m_events.begin() + eventToEraseIdx);
            }
        }

        ImGui::PopID(); // Event track
    }

    // -- Last row
    ImGui::TableNextRow();
    {
        ImGui::TableNextColumn();
        {
            ImGui::Text("Add Track");
            ImGui::SameLine();
            if (ImGui::SmallButton(ICON_FA_PLUS))
            {
                auto pTrack = aln::New<EditorEventTrack>();
                m_eventTracks.push_back(pTrack);
            }
        }

        ImGui::TableNextColumn();
        {
            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, sequencerBodyBgColor);
            
            // --- Draw vertical indicators
            // They are drawn in a column to take advantage of the auto-clipping
            DrawTimelineMarker(timelineDrawingArea, m_viewRange, 0.0f, RGBColor::Green);
            DrawTimelineMarker(timelineDrawingArea, m_viewRange, m_pAnimationClip->GetFrameCount() - 1, RGBColor::Red);
            DrawTimelineMarker(timelineDrawingArea, m_viewRange, m_pAnimationPlayerComponent->GetPercentageThroughAnimation() * (m_pAnimationClip->GetFrameCount() - 1), RGBColor::Blue);

            auto remainingY = ImGui::GetWindowContentRegionMax().y - ImGui::GetCursorPosY() - ImGui::GetStyle().FramePadding.y;
            ImGui::Dummy({0, remainingY});
        }
    }

    if (ImGui::TableGetHoveredColumn() == 1)
    {
        if (ImGui::GetIO().KeyCtrl)
        {
            constexpr float zoomSpeed = 2.0f;
            // TODO: Scale with timeline view range

            const float zoom = ImGui::GetIO().MouseWheel;
            const float mousePosX = ImGui::GetMousePos().x;

            const float relativeMousePosX = Maths::InverseLerp(timelineDrawingArea.Min.x, timelineDrawingArea.Max.x, mousePosX);
            m_viewRange.m_start += zoom * zoomSpeed * relativeMousePosX;
            m_viewRange.m_end -= zoom * zoomSpeed * (1.0f - relativeMousePosX);
        }

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
        {
            m_timelinePanningActive = true;
        }
    }
    ImGui::EndTable();

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
    {
        m_timelinePanningActive = false;
    }

    if (m_timelinePanningActive)
    {
        constexpr float scrollSpeed = 0.2f;
        // TODO: Scale with timeline view range

        const float mouseDeltaX = ImGui::GetIO().MouseDelta.x;
        m_viewRange.m_start -= mouseDeltaX * scrollSpeed;
        m_viewRange.m_end -= mouseDeltaX * scrollSpeed;
    }

    // Handle requests
    if (trackToEraseIdx != InvalidIndex)
    {
        aln::Delete(m_eventTracks[trackToEraseIdx]);
        m_eventTracks.erase(m_eventTracks.begin() + trackToEraseIdx);
    }
}

void AnimationClipWorkspace::Update(const UpdateContext& context)
{
    static float previewHeight = 300;
    static float trackHeight = 300;

    // Late initialization after the anim clip is loaded
    if (m_pAnimationClip.IsLoaded() && !m_pPreviewCharacterSkeletalMeshComponent->HasSkeletonSet())
    {
        m_pPreviewWorld->StartComponentEditing((IComponent*) m_pPreviewCharacterSkeletalMeshComponent);
        m_pPreviewCharacterSkeletalMeshComponent->SetSkeleton(m_pAnimationClip->GetSkeleton()->GetID());
        m_pPreviewWorld->EndComponentEditing(m_pPreviewCharacterSkeletalMeshComponent);

        m_playbackRange = {0.0f, (float) m_pAnimationClip->GetFrameCount() - 1};
        m_viewRange = m_playbackRange;
    }

    if (ImGui::Begin((std::string(ICON_FA_PERSON_RUNNING " ") + GetID().GetAssetName() + "##" + GetID().GetAssetPath()).c_str(), &m_isOpen))
    {
        float contentWidth = ImGui::GetContentRegionAvail().x;
        ImGuiWidgets::Splitter(false, 2, &previewHeight, &trackHeight, 8, 8, contentWidth);

        if (ImGui::BeginChild("Animation Preview", {contentWidth, previewHeight}, false))
        {
            // Update current scene preview dims
            auto dim = ImGui::GetContentRegionAvail();
            m_pPreviewWorld->UpdateViewportSize(dim.x, dim.y);

            auto& descriptor = m_pPreviewWorld->GetSystem<WorldRenderingSystem>()->GetGPUResources().m_resolveImage.GetDescriptorSet();
            ImGui::Image((ImTextureID) descriptor, dim);

            ImGui::EndChild();
        }

        if (ImGui::BeginChild("Event Tracks", {contentWidth, -1}, false))
        {
            if (m_pAnimationClip.IsLoaded())
            {
                DrawAnimationPlaybackController();
                DrawAnimationEventsEditor();
            }
            ImGui::EndChild();
        }
    }
    ImGui::End();

    if (!m_isOpen)
    {
        RequestAssetWindowDeletion(GetID());
    }
}

void AnimationClipWorkspace::Initialize(EditorWindowContext* pContext, const AssetID& id, bool readAssetFile)
{
    IAssetWorkspace::Initialize(pContext, id, readAssetFile);

    m_pAnimationClip = AssetHandle<AnimationClip>(id);
    if (readAssetFile)
    {
        RequestAssetLoad(m_pAnimationClip);
    }

    IAssetWorkspace::CreatePreviewWorld();
    m_pPreviewWorld->CreateSystem<WorldRenderingSystem>();

    // -- Camera
    m_pPreviewCameraEntity = m_pPreviewWorld->CreateEntity("Camera");

    auto pCameraComponent = aln::New<CameraComponent>();
    m_pPreviewCameraEntity->AddComponent(pCameraComponent);
    // TODO: Place the camera a lil bit better by default

    m_pPreviewCameraEntity->CreateSystem<EditorCameraControllerSystem>();

    // -- Floor
    auto pFloorEntity = m_pPreviewWorld->CreateEntity("Floor");

    auto pFloorMeshComponent = aln::New<StaticMeshComponent>();
    pFloorMeshComponent->SetMesh(AssetID(PreviewSceneFloorMeshAssetFilepath));
    pFloorEntity->AddComponent(pFloorMeshComponent);

    // TODO: Add lights

    // -- Preview character
    auto pCharacterEntity = m_pPreviewWorld->CreateEntity("Character");

    m_pPreviewCharacterSkeletalMeshComponent = aln::New<SkeletalMeshComponent>();
    // TODO: Auto-select a matching skeletal mesh from the asset db (when there's an asset db)
    pCharacterEntity->AddComponent(m_pPreviewCharacterSkeletalMeshComponent);

    m_pAnimationPlayerComponent = aln::New<AnimationPlayerComponent>();
    m_pAnimationPlayerComponent->SetAnimationClip(m_pAnimationClip.GetAssetID());
    m_pAnimationPlayerComponent->SetPaused(true);
    pCharacterEntity->AddComponent(m_pAnimationPlayerComponent);

    pCharacterEntity->CreateSystem<AnimationSystem>();
}

void aln::AnimationClipWorkspace::Shutdown()
{
    DeletePreviewWorld();

    m_pPreviewCameraEntity = nullptr;
    m_pAnimationPlayerComponent = nullptr;
    m_pPreviewCharacterSkeletalMeshComponent = nullptr;
    m_pAnimationPlayerComponent = nullptr;

    IAssetWorkspace::Shutdown();
}

void aln::AnimationClipWorkspace::Clear()
{
}

AnimationClip* aln::AnimationClipWorkspace::Compile()
{
    assert(false); // TODO
    return nullptr;
}
} // namespace aln