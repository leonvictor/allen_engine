#include "assets/animation_clip_workspace.hpp"
#include "aln_imgui_widgets.hpp"

#include <common/maths/maths.hpp>
#include <core/components/animation_player_component.hpp>
#include <core/components/camera.hpp>
#include <core/entity_systems/camera_controller.hpp>
#include <core/world_systems/world_rendering_system.hpp>
#include <core/entity_systems/animation_system.hpp>

#include <IconsFontAwesome6.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

namespace aln
{

static bool SliderTimeline(ImGuiID id, float* pCurrentTime, const float animDuration, ImRect* pOutSliderDrawingArea, ImVec2* pOutSliderCursorPosition)
{
    ImGuiWindow* pWindow = ImGui::GetCurrentWindow();
    if (pWindow->SkipItems)
    {
        return false;
    }

    auto pContext = ImGui::GetCurrentContext();

    const auto windowPos = ImGui::GetWindowPos();
    const auto cursorPos = ImGui::GetCursorPos();
    const auto contentRegionAvail = ImGui::GetContentRegionAvail();

    const auto timelineHeight = 40;

    const auto timelineTL = windowPos + cursorPos;
    const auto timelineBR = timelineTL + ImVec2(contentRegionAvail.x, timelineHeight);

    const ImRect timelineBB = {
        timelineTL,
        timelineBR,
    };

    ImGui::ItemSize(timelineBB);
    if (!ImGui::ItemAdd(timelineBB, id, &timelineBB))
    {
        return false;
    }

    const bool hovered = ImGui::ItemHoverable(timelineBB, id, ImGuiItemFlags_None);
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
    const bool valueChanged = ImGui::SliderBehaviorT<float, float, float>(timelineBB, id, ImGuiDataType_Float, pCurrentTime, 0, animDuration, "", ImGuiSliderFlags_None, &cursorGrabBB);
    if (valueChanged)
    {
        ImGui::MarkItemEdited(id);
    }

    // ImGui::GetForegroundDrawList()->AddRect(timelineBB.Min, timelineBB.Max, RGBColor::Pink.ToU32());
    // ImGui::GetForegroundDrawList()->AddRect(cursorGrabBB.Min, cursorGrabBB.Max, RGBColor::Red.ToU32());

    // Draw timeline
    auto pDrawList = ImGui::GetWindowDrawList();

    float horizontalPadding = cursorGrabBB.GetWidth() / 2.0f;

    ImRect drawingArea = {
        {timelineBB.Min.x + horizontalPadding + 1.0f, timelineBB.Min.y},
        {timelineBB.Max.x - horizontalPadding - 2.0f, timelineBB.Max.y},
    };

    const auto majorGraduationOffset = drawingArea.GetWidth() / animDuration;
    const auto minorGraduationOffset = majorGraduationOffset / 10.0f;

    for (auto seconds = 0; seconds <= animDuration; ++seconds)
    {
        // Major graduations
        const auto majorGraduationPosX = drawingArea.Min.x + (seconds * majorGraduationOffset);
        const auto majorGraduationPosY = drawingArea.Min.y;
        pDrawList->AddLine({majorGraduationPosX, majorGraduationPosY}, {majorGraduationPosX, majorGraduationPosY + timelineHeight}, IM_COL32_WHITE);
        pDrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize(), {majorGraduationPosX + 5.0f, majorGraduationPosY}, IM_COL32_WHITE, std::to_string(seconds).c_str());

        // Minor graduations
        for (auto ms = 1; ms < 10; ++ms)
        {
            if (seconds + (ms * 0.1f) > animDuration)
            {
                break;
            }

            const auto minorGraduationPosX = majorGraduationPosX + (ms * minorGraduationOffset);
            pDrawList->AddLine({minorGraduationPosX, majorGraduationPosY + timelineHeight / 2}, {minorGraduationPosX, majorGraduationPosY + timelineHeight}, IM_COL32_WHITE);
        }
    }

    // Render slider cursor
    const auto sliderCursorWidth = 10.0f;
    const auto sliderCursorHeight = timelineHeight / 2;
    const auto sliderCursorPos = cursorGrabBB.GetCenter();

    const ImVec2 cursorPoints[] = {
        {sliderCursorPos.x - sliderCursorWidth / 2, sliderCursorPos.y},
        {sliderCursorPos.x + sliderCursorWidth / 2, sliderCursorPos.y},
        {sliderCursorPos.x + sliderCursorWidth / 2, sliderCursorPos.y + sliderCursorHeight / 2},
        {sliderCursorPos.x, sliderCursorPos.y + sliderCursorHeight},
        {sliderCursorPos.x - sliderCursorWidth / 2, sliderCursorPos.y + sliderCursorHeight / 2}};

    pDrawList->AddConvexPolyFilled(cursorPoints, 5, IM_COL32_WHITE);

    if (pOutSliderCursorPosition != nullptr)
    {
        *pOutSliderCursorPosition = sliderCursorPos;
    }

    if (pOutSliderDrawingArea != nullptr)
    {
        *pOutSliderDrawingArea = drawingArea;
    }

    return valueChanged;
}

void AnimationClipWorkspace::DrawAnimationPreview()
{
}

void AnimationClipWorkspace::DrawAnimationEventsEditor()
{
    float animDuration = 5; // TMP. Animation duration in seconds

    /// --------- Header
    // Anim player control
    ImGui::Text(ICON_FA_BACKWARD_STEP);
    ImGui::SameLine();
    ImGui::Text(ICON_FA_PLAY);
    ImGui::SameLine();
    ImGui::Text(ICON_FA_FORWARD_STEP);
    ImGui::SameLine();
    ImGui::Text("%.2f / %.2fs", m_animationTime, animDuration);

    ImVec2 timelineCursorPos;
    ImRect timelineDrawingArea;

    auto TimelineDeltaToAnimTime = [&](float timelineDelta)
    { return (animDuration / timelineDrawingArea.GetWidth()) * timelineDelta; };

    auto AnimTimeToTimelinePosition = [&](float animTime)
    { return timelineDrawingArea.Min.x + animTime * (timelineDrawingArea.GetWidth() / animDuration); };

    ImGui::BeginTable("EventTable", 2, ImGuiTableFlags_Resizable);

    ImGui::TableNextRow();
    {
        ImGui::TableNextColumn();
        {
            // TODO: Header
        }

        // Timeline
        ImGui::TableNextColumn();
        {
            SliderTimeline(ImGui::GetID("Animation Event Timeline"), &m_animationTime, animDuration, &timelineDrawingArea, &timelineCursorPos);
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
        }

        // Track details
        ImGui::TableNextColumn();
        {
            constexpr float trackRowHeight = 20;
            auto columnCursorPos = ImGui::GetCursorScreenPos();
            auto columnRegion = ImGui::GetContentRegionAvail();

            ImGui::SetNextItemAllowOverlap();
            ImGui::InvisibleButton("track details", {ImGui::GetContentRegionAvail().x, trackRowHeight});
            if (ImGui::BeginPopupContextItem())
            {
                // TODO: Prevent adding event at the same time
                if (ImGui::MenuItem("Add event"))
                {
                    auto pEvent = aln::New<EditorAnimationEvent>();
                    pEvent->m_startTime = m_animationTime;
                    pEvent->m_endTime = m_animationTime;
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

                if (!Maths::IsNearEqual(pEvent->m_startTime, pEvent->m_endTime, EditorAnimationEvent::MinDurationEpsilon))
                {
                    ImGui::PushID("End Event");

                    ImVec2 endEventPosition = {
                        AnimTimeToTimelinePosition(pEvent->m_endTime),
                        columnCursorPos.y + trackRowHeight * 0.5f,
                    };

                    // Draw durable event bar
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
                        if (maxTime > animDuration)
                        {
                            auto diff = maxTime - animDuration;
                            pEvent->m_startTime -= diff;
                            pEvent->m_endTime -= diff;
                        }
                    }

                    // Draw event knob
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

                    ImGui::PopID();
                }

                // Enforce anim duration boundaries
                pEvent->m_startTime = Maths::Clamp(pEvent->m_startTime, 0.0f, animDuration);
                pEvent->m_endTime = Maths::Clamp(pEvent->m_endTime, 0.0f, animDuration);

                ImGui::PopID();
            }

            if (eventToEraseIdx != InvalidIndex)
            {
                aln::Delete(pEventTrack->m_events[eventToEraseIdx]);
                pEventTrack->m_events.erase(pEventTrack->m_events.begin() + eventToEraseIdx);
            }
        }

        ImGui::PopID();
    }

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
    }

    ImGui::EndTable();

    // Cursor line
    auto lineEnd = ImVec2(timelineCursorPos.x, ImGui::GetWindowPos().y + ImGui::GetCursorPosY() - (ImGui::GetStyle().FramePadding.y * 2));
    ImGui::GetWindowDrawList()->AddLine(timelineCursorPos, lineEnd, static_cast<uint32_t>(RGBColor::Blue));

    // Handle requests
    if (trackToEraseIdx != InvalidIndex)
    {
        aln::Delete(m_eventTracks[trackToEraseIdx]);
        m_eventTracks.erase(m_eventTracks.begin() + trackToEraseIdx);
    }

    // TODO: Draw grid
    // TODO: Highlight selected
}

void AnimationClipWorkspace::Update(const UpdateContext& context)
{
    static float previewHeight = 300;
    static float trackHeight = 300;

    if (m_pAnimationClip.IsLoaded() && !m_pPreviewCharacterSkeletalMeshComponent->HasSkeletonSet())
    {
        m_pPreviewWorld->StartComponentEditing((IComponent*) m_pPreviewCharacterSkeletalMeshComponent);
        m_pPreviewCharacterSkeletalMeshComponent->SetSkeleton(m_pAnimationClip->GetSkeleton()->GetID());
        m_pPreviewWorld->EndComponentEditing(m_pPreviewCharacterSkeletalMeshComponent);
    }

    if (ImGui::Begin("Animation Clip", &m_isOpen))
    {
        float contentWidth = ImGui::GetContentRegionAvail().x;
        ImGuiWidgets::Splitter(false, 2, &previewHeight, &trackHeight, 8, 8, contentWidth);

        if (ImGui::BeginChild("Animation Preview", {contentWidth, previewHeight}, true))
        {
            // TODO: Animation preview

            // Update current scene preview dims
            auto dim = ImGui::GetContentRegionAvail();
            m_pPreviewWorld->UpdateViewportSize(dim.x, dim.y);

            auto& descriptor = m_pPreviewWorld->GetSystem<WorldRenderingSystem>()->GetGPUResources().m_resolveImage.GetDescriptorSet();
            ImGui::Image((ImTextureID) descriptor, dim);

            ImGui::EndChild();
        }

        if (ImGui::BeginChild("Event Tracks", {contentWidth, -1}, true))
        {
            DrawAnimationEventsEditor();
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

    m_pPreviewCameraEntity->CreateSystem<EditorCameraController>();

    // -- Floor
    auto pFloorEntity = m_pPreviewWorld->CreateEntity("Floor");

    auto pFloorMeshComponent = aln::New<StaticMeshComponent>();
    pFloorMeshComponent->SetMesh(AssetID(PreviewSceneFloorMeshAssetFilepath));
    pFloorEntity->AddComponent(pFloorMeshComponent);

    // TODO: Add lights

    // -- Preview character
    auto pCharacterEntity = m_pPreviewWorld->CreateEntity("Character");

    m_pPreviewCharacterSkeletalMeshComponent = aln::New<SkeletalMeshComponent>();
    //m_pPreviewCharacterSkeletalMeshComponent->SetMesh(m_previewSceneSettings.m_pSkeletalMesh.GetAssetID());
    pCharacterEntity->AddComponent(m_pPreviewCharacterSkeletalMeshComponent);

    m_pAnimationPlayerComponent = aln::New<AnimationPlayerComponent>();
    m_pAnimationPlayerComponent->SetAnimationClip(m_pAnimationClip.GetAssetID());
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