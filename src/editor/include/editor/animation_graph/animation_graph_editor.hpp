#pragma once

#include <imgui.h>
#include <imnodes.h>

namespace aln
{
class AnimationGraphEditor
{
  public:
    static void Draw()
    {
        ImNodes::BeginNodeEditor();
        ImNodes::EndNodeEditor();
    }
};
} // namespace aln