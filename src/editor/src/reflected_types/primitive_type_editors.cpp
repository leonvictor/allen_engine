#include "reflected_types/primitive_type_editors.hpp"

#include <assets/asset.hpp>
#include <assets/handle.hpp>

#include <imgui.h>
#include <imgui_stdlib.h>

namespace aln
{

// ------ Concrete primitive types editors

class IntEditor : public IPrimitiveTypeEditor
{
    int m_editingValue;
    int* m_pInstanceValue;

    bool DrawWidget(std::byte* pTypeInstance) override
    {
        m_pInstanceValue = reinterpret_cast<int*>(pTypeInstance);
        m_editingValue = *m_pInstanceValue;

        return ImGui::InputInt("##Int", &m_editingValue);
    }

    void UpdateValue() override
    {
        *m_pInstanceValue = m_editingValue;
    }
};

class FloatEditor : public IPrimitiveTypeEditor
{
    float m_editingValue;
    float* m_pInstanceValue;

    bool DrawWidget(std::byte* pTypeInstance) override
    {
        m_pInstanceValue = reinterpret_cast<float*>(pTypeInstance);
        m_editingValue = *m_pInstanceValue;

        return ImGui::DragFloat("##Float", &m_editingValue);
    }

    void UpdateValue() override
    {
        *m_pInstanceValue = m_editingValue;
    }
};

class BoolEditor : public IPrimitiveTypeEditor
{
    bool m_editingValue;
    bool* m_pInstanceValue;

    bool DrawWidget(std::byte* pTypeInstance) override
    {
        m_pInstanceValue = reinterpret_cast<bool*>(pTypeInstance);
        m_editingValue = *m_pInstanceValue;

        return ImGui::Checkbox("##Bool", &m_editingValue);
    }

    void UpdateValue() override
    {
        *m_pInstanceValue = m_editingValue;
    }
};

class Color4Editor : public IPrimitiveTypeEditor
{
    RGBAColor m_editingValue;
    RGBAColor* m_pInstanceValue;

    bool DrawWidget(std::byte* pTypeInstance) override
    {
        m_pInstanceValue = reinterpret_cast<RGBAColor*>(pTypeInstance);
        m_editingValue = *m_pInstanceValue;

        return ImGui::ColorEdit4("##Color4", (float*) &m_editingValue);
    }

    void UpdateValue() override
    {
        *m_pInstanceValue = m_editingValue;
    }
};

class Color3Editor : public IPrimitiveTypeEditor
{
    RGBColor m_editingValue;
    RGBColor* m_pInstanceValue;

    bool DrawWidget(std::byte* pTypeInstance) override
    {
        m_pInstanceValue = reinterpret_cast<RGBColor*>(pTypeInstance);
        m_editingValue = *m_pInstanceValue;

        return ImGui::ColorEdit3("##Color3", (float*) &m_editingValue);
    }

    void UpdateValue() override
    {
        *m_pInstanceValue = m_editingValue;
    }
};

class AssetIDEditor : public IPrimitiveTypeEditor
{
    friend class AssetHandleEditor;

    AssetID m_editingValue;
    AssetID* m_pInstanceValue;

    bool DrawWidget(std::byte* pTypeInstance)
    {
        m_pInstanceValue = reinterpret_cast<AssetID*>(pTypeInstance);
        m_editingValue = *m_pInstanceValue;

        bool valueChanged = false;
        auto assetName = m_editingValue.IsValid() ? m_editingValue.GetAssetName() : "None";

        ImGui::InputText("##AssetID", &assetName, ImGuiInputTextFlags_ReadOnly);

        if (m_editingValue.IsValid() && ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(m_editingValue.GetAssetPath().c_str());
            ImGui::EndTooltip();
        }

        if (ImGui::BeginDragDropTarget())
        {
            // TODO: Associate payload with asset type
            if (const ImGuiPayload* pPayload = ImGui::AcceptDragDropPayload("AssetID"))
            {
                assert(pPayload->DataSize == sizeof(AssetID));
                m_editingValue = *((AssetID*) pPayload->Data);
                valueChanged = true;
            }
            ImGui::EndDragDropTarget();
        }
        return valueChanged;
    }

    void UpdateValue() override
    {
        *m_pInstanceValue = m_editingValue;
    }

    virtual bool RequireEntityReload() const override { return true; }
};

class AssetHandleEditor : public IPrimitiveTypeEditor
{
    // TODO: Used a cached instance
    AssetIDEditor m_assetIDEditor;

    AssetHandle<IAsset> m_editingValue;
    AssetHandle<IAsset>* m_pInstanceValue;

    bool DrawWidget(std::byte* pTypeInstance) override
    {
        m_pInstanceValue = reinterpret_cast<AssetHandle<IAsset>*>(pTypeInstance);
        m_editingValue = *m_pInstanceValue;

        // TODO: Reload as necessary
        // TODO: Notify the owning entity
        return m_assetIDEditor.DrawWidget((std::byte*) const_cast<AssetID*>(&m_editingValue.GetAssetID()));
    }

    void UpdateValue() override
    {
        m_assetIDEditor.UpdateValue();
        *m_pInstanceValue = m_editingValue;
    }

    virtual bool RequireEntityReload() const override { return true; }
};

// ------ Factory

IPrimitiveTypeEditor* IPrimitiveTypeEditor::CreateEditor(const StringID& typeID)
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
} // namespace aln