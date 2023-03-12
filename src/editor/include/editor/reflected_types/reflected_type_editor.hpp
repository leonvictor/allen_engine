#pragma once

#include "primitive_type_editors.hpp"

#include <reflection/type_info.hpp>
#include <unordered_map>

// TODO: Move to implementation
#include <assets/asset.hpp>
#include <assets/handle.hpp>

namespace aln
{
class ReflectedTypeEditor
{
  private:
    static std::unordered_map<StringID, IPrimitiveTypeEditor*> PrimitiveTypeEditors;

    /// @brief Factory method creating editor fields for primitive types
    static IPrimitiveTypeEditor* CreatePrimitiveTypeEditor(const StringID& typeID)
    {
        if (typeID == aln::reflect::TypeInfoResolver<int>::Get()->GetTypeID())
        {
            return aln::New<IntEditor>();
        }
        if (typeID == aln::reflect::TypeInfoResolver<bool>::Get()->GetTypeID())
        {
            return aln::New<BoolEditor>();
        }
        if (typeID == aln::reflect::TypeInfoResolver<AssetHandle<IAsset>>::Get()->GetTypeID())
        {
            return aln::New<AssetHandleEditor>();
        }
        if (typeID == aln::reflect::TypeInfoResolver<AssetID>::Get()->GetTypeID())
        {
            return aln::New<AssetIDEditor>();
        }
        if (typeID == aln::reflect::TypeInfoResolver<float>::Get()->GetTypeID())
        {
            return aln::New<FloatEditor>();
        }
        if (typeID == aln::reflect::TypeInfoResolver<RGBColor>::Get()->GetTypeID())
        {
            return aln::New<Color3Editor>();
        }
        if (typeID == aln::reflect::TypeInfoResolver<RGBAColor>::Get()->GetTypeID())
        {
            return aln::New<Color4Editor>();
        }
    }

    static IPrimitiveTypeEditor* GetOrCreatePrimitiveTypeEditor(const StringID& typeID)
    {
        auto [it, emplaced] = PrimitiveTypeEditors.try_emplace(typeID);
        if (emplaced)
        {
            it->second = CreatePrimitiveTypeEditor(typeID);
        }
        return it->second;
    }

  public:
    static void Initialize();
    static void Shutdown();

    /// @brief Parse the reflected fields of a custom reflected class to display them in editor
    /// @param widgetColumnWidth Desired widget column width. Default behavior is to use two third of the available window width.
    static void Draw(const reflect::TypeInfo* pTypeInfo, void* pTypeInstance, float widgetColumnWidth = -FLT_MIN)
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
                auto pEditor = GetOrCreatePrimitiveTypeEditor(memberInfo.GetTypeID());
                pEditor->Draw(pMemberInstance);

                ImGui::PopItemWidth();

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::PopID();
    }
};
}; // namespace aln