#pragma once

#include <common/uuid.hpp>

namespace aln
{

class EditorGraphNode;

class Link
{
  public:
    UUID m_id = UUID::Generate();
    UUID m_inputPinID;
    const EditorGraphNode* m_pInputNode = nullptr;
    UUID m_outputPinID;
    const EditorGraphNode* m_pOutputNode = nullptr;

    const UUID& GetID() const { return m_id; }
};
} // namespace aln