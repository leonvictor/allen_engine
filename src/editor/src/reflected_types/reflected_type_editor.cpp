#include "reflected_types/reflected_type_editor.hpp"

#include <assets/asset.hpp>
#include <assets/handle.hpp>
#include <reflection/reflected_type.hpp>
#include <reflection/type_info.hpp>

#include <imgui.h>

namespace aln
{
HashMap<StringID, IPrimitiveTypeEditor*> ReflectedTypeEditor::PrimitiveTypeEditors;

IPrimitiveTypeEditor* ReflectedTypeEditor::GetOrCreatePrimitiveTypeEditor(const StringID& typeID)
{
    auto [it, emplaced] = PrimitiveTypeEditors.try_emplace(typeID);
    if (emplaced)
    {
        it->second = IPrimitiveTypeEditor::CreateEditor(typeID);
    }
    return it->second;
}

void ReflectedTypeEditor::Draw(const reflect::TypeInfo* pTypeInfo, void* pTypeInstance, float widgetColumnWidth) const
{
    assert(pTypeInfo != nullptr && pTypeInstance != nullptr);

    ImGui::PushID(pTypeInstance);
    if (pTypeInfo->GetMemberCount() > 0 && ImGui::BeginTable("##MemberTable", 2, ImGuiTableFlags_SizingFixedFit))
    {
        if (widgetColumnWidth == -FLT_MIN)
        {
            ImGui::TableSetupColumn("##MemberName", ImGuiTableColumnFlags_WidthStretch, 0.3);
            ImGui::TableSetupColumn("##Widget", ImGuiTableColumnFlags_WidthStretch, 0.7);
        }

        for (const auto& memberInfo : pTypeInfo->m_members)
        {
            ImGui::PushID(memberInfo.GetName().c_str());

            ImGui::TableNextRow();

            // Draw name
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();

            ImGui::Text(memberInfo.GetPrettyName().c_str());

            // Draw widget
            ImGui::TableNextColumn();
            ImGui::PushItemWidth(widgetColumnWidth);

            std::byte* pMemberInstance = (std::byte*) pTypeInstance + memberInfo.GetOffset();

            // TODO: Move Factory to a primitive-only class
            auto pEditor = GetOrCreatePrimitiveTypeEditor(memberInfo.GetTypeID());

            if (pEditor->DrawWidget(pMemberInstance))
            {
                TypeEditedEventDetails details;
                details.m_action = pEditor->RequireEntityReload() ? TypeEditedEventDetails::Action::EditRequiresReload : TypeEditedEventDetails::Action::Edit;
                details.m_pEditedMemberInfo = &memberInfo;
                details.m_pEditedTypeInstance = reinterpret_cast<reflect::IReflected*>(pTypeInstance);

                m_typeEditionStartedEvent.Fire(details);
                pEditor->UpdateValue();
                m_typeEditionCompletedEvent.Fire(details);
            }

            ImGui::PopItemWidth();

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::PopID();
}

void ReflectedTypeEditor::Shutdown()
{
    for (auto& [id, pEditor] : PrimitiveTypeEditors)
    {
        aln::Delete(pEditor);
    }
    PrimitiveTypeEditors.clear();
}
} // namespace aln