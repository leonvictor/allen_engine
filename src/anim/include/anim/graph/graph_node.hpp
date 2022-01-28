#pragma once

#include <assert.h>

#include "graph_context.hpp"

namespace aln
{

struct Target
{
    // TODO
};

// todo: where ?
enum class NodeValueType : uint8_t
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
  public:
    class Runtime
    {
 // TODO: Pointer to the runtime graph DEFINITION, which holds the settings values
    };
    class Settings
    {
      protected:
        /// @brief Create a node, add it to the list and return a pointer to it
        /// @param nodePtrs: Pre-allocated node memory chunk
        /// @param options: TODO
        template <typename T>
        T::Runtime* CreateNode(const std::vector<GraphNode*>& nodePtrs, InitOptions options) const
        {
            // TODO: What do we do with the options ?
            // TODO: The vector is pre-allocated. Placement-new a node of type T in there !
            // TODO: Does emplace_back(new T(...)) use placement-new ?

            // TODO: nodePtrs is const. How do we insert in it ?
            // GraphNode* ptr = nodePtrs.emplace_back(new T);
            T* ptr;
            return static_cast<T*>(ptr);
        }

        /// @brief Set a node based on a given index, only if the index was set
        void SetOptionalNodePtrFromIndex(const std::vector<GraphNode*>& nodePtrs, const NodeIndex nodeIndex, GraphNode* pNode) const
        {
            if (nodeIndex == InvalidIndex)
                return;

            assert(nodeIndex < nodePtrs.size());
            pNode = nodePtrs[InvalidIndex];
        }

      public:
        virtual void InstanciateNode(const std::vector<GraphNode*>& nodePtrs, AnimationGraphDataSet const* pDataSet, InitOptions options) const = 0;
    };

  protected:
    bool IsNodeActive(GraphContext& context) const
    {
        // TODO
    }

    virtual void Initialize(GraphContext& context) = 0;                            // ?
    virtual void InitializeInternal(GraphContext& context) = 0;                    // ?
    virtual NodeValueType GetValueType() const { return NodeValueType::Unknown; }; // ?

  private:
    GraphNode::Settings* m_settings; // TODO

  public:
    template <typename T>
    T::Settings* GetSettings() const
    {
        // TODO: Runtime nodes do not contain data, and instead point to the memory where the "graph definition" lies
        return static_cast<T::Settings*>(m_settings);
    }
};

} // namespace aln