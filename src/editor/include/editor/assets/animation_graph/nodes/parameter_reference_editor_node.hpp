#pragma once

#include "assets/animation_graph/editor_animation_graph_node.hpp"

namespace aln
{
class IControlParameterEditorNode;

/// @brief References an existing control parameter
class ParameterReferenceEditorNode : public EditorAnimationGraphNode
{
    ALN_REGISTER_TYPE()

    friend class EditorGraph;

  private:
    const IControlParameterEditorNode* m_pParameter = nullptr;
    StringID m_parameterID = StringID::InvalidID;

  public:
    ParameterReferenceEditorNode() = default;
    ParameterReferenceEditorNode(const IControlParameterEditorNode* pReferencedParameter);

    void Initialize() override;
    NodeIndex Compile(AnimationGraphCompilationContext& context, AnimationGraphDefinition& graphDefinition) const override;

    const std::string& GetName() const override;
    const IControlParameterEditorNode* GetReferencedParameter() const { return m_pParameter; }
    const StringID& GetReferencedParameterID() const { return m_parameterID; }

    void SaveState(JSON& json) const override;
    void LoadState(const JSON& json, const TypeRegistryService* pTypeRegistryService) override;
};
} // namespace aln