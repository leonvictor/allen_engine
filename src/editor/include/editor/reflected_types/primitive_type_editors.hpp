#pragma once

#include <cstddef>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <assets/asset.hpp>
#include <assets/handle.hpp>

namespace aln
{

class IPrimitiveTypeEditor
{
  public:
    virtual bool Draw(std::byte* pTypeInstance) = 0;
};

class IntEditor : public IPrimitiveTypeEditor
{
    bool Draw(std::byte* pTypeInstance) override
    {
        int* pValue = reinterpret_cast<int*>(pTypeInstance);
        return ImGui::InputInt("##Int", pValue);
    }
};

class FloatEditor : public IPrimitiveTypeEditor
{
    bool Draw(std::byte* pTypeInstance) override
    {
        float* pValue = reinterpret_cast<float*>(pTypeInstance);
        return ImGui::DragFloat("##Float", pValue);
    }
};

class BoolEditor : public IPrimitiveTypeEditor
{
    bool Draw(std::byte* pTypeInstance) override
    {
        bool* pValue = reinterpret_cast<bool*>(pTypeInstance);
        return ImGui::Checkbox("##Bool", pValue);
    }
};

class Color4Editor : public IPrimitiveTypeEditor
{
    bool Draw(std::byte* pTypeInstance) override
    {
        RGBAColor* pValue = reinterpret_cast<RGBAColor*>(pTypeInstance);
        return ImGui::ColorEdit4("##Color4", (float*) pValue);
    }
};

class Color3Editor : public IPrimitiveTypeEditor
{
    bool Draw(std::byte* pTypeInstance) override
    {
        RGBColor* pValue = reinterpret_cast<RGBColor*>(pTypeInstance);
        return ImGui::ColorEdit3("##Color3", (float*) pValue);
    }
};

class AssetIDEditor : public IPrimitiveTypeEditor
{
    friend class AssetHandleEditor;

    static bool StaticDraw(std::byte* pTypeInstance)
    {
        bool valueChanged = false;

        auto pValue = reinterpret_cast<AssetID*>(pTypeInstance);
        auto assetName = pValue->IsValid() ? pValue->GetAssetName() : "None";

        ImGui::InputText("##AssetID", &assetName, ImGuiInputTextFlags_ReadOnly);

        if (pValue->IsValid() && ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(pValue->GetAssetPath().c_str());
            ImGui::EndTooltip();
        }

        if (ImGui::BeginDragDropTarget())
        {
            // TODO: Associate payload with asset type
            if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("AssetID"))
            {
                assert(pPayload->DataSize == sizeof(AssetID));

                // TODO:
                // *pID = *((AssetID*) pPayload->Data);

                valueChanged = true;
            }
            ImGui::EndDragDropTarget();
        }
        return valueChanged;
    }

    bool Draw(std::byte* pTypeInstance) override
    {
        return StaticDraw(pTypeInstance);
    }
};

class AssetHandleEditor : public IPrimitiveTypeEditor
{
    bool Draw(std::byte* pTypeInstance) override
    {
        auto pValue = reinterpret_cast<AssetHandle<IAsset>*>(pTypeInstance);
        return AssetIDEditor::StaticDraw((std::byte*) const_cast<AssetID*>(&pValue->GetAssetID()));
    }
};
} // namespace aln