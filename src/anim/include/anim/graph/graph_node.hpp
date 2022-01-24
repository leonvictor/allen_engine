#pragma once

#include <assert.h>

#include "graph_context.hpp"

namespace aln
{
// TODO
struct Target
{
};
// todo: where ?
enum class NodeValueType
{
    Unknown,
    Pose,
    Bool,
    ID,
    Int,
    Float,
    Vector,
    Target,
    BoneMask,
};

struct InitOptions
{
}; // TODO

class GraphNode
{
  protected:
    // TODO
    struct Settings
    {
        template <typename T>
        T* CreateNode(const std::vector<GraphNode*>& nodePtrs, InitOptions options)
        {
            //TODO
        }

        void SetOptionalNodePtrFromIndex(const std::vector<GraphNode*>& nodePtrs, uint32_t nodeIndex, GraphNode* pNode)
        {
            // TODO
        }
    };

    bool IsNodeActive(GraphContext& context) const;

    virtual void Initialize(GraphContext& context) = 0;                            // ?
    virtual void InitializeInternal(GraphContext& context) = 0;                    // ?
    virtual NodeValueType GetValueType() const { return NodeValueType::Unknown; }; // ?

  public:
    template <typename T>
    T::Settings* GetSettings() const
    {
        // TODO
    }
};

} // namespace aln