#pragma once

#include <common/uuid.hpp>

#include "editor_graph_node.hpp"
#include "pin.hpp"

namespace aln
{
class Link
{
  public:
    UUID m_id = UUID::Generate();
    UUID m_inputPinID;
    EditorGraphNode* m_pInputNode = nullptr;
    UUID m_outputPinID;
    EditorGraphNode* m_pOutputNode = nullptr;
};
} // namespace aln