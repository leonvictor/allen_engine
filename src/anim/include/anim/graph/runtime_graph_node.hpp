#pragma once

#include <assert.h>

#include <common/serialization/binary_archive.hpp>
#include <reflection/reflected_type.hpp>

#include "animation_graph_dataset.hpp"
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

enum class InitOptions : uint8_t
{
    None,
}; // TODO

/// @brief Base class for runtime nodes. Nodes do not contain data themselves, all of it is kept in the settings
class RuntimeGraphNode
{
    friend class Settings;

  protected:
    enum class Status : uint8_t
    {
        Uninitialized,
        Initialized,
    };

  public:
    class Settings : public reflect::IReflected
    {
        ALN_REGISTER_TYPE();

        friend class AnimationGraphCompilationContext;

      private:
        uint32_t m_nodeIndex = InvalidIndex; // Index of the node in the runtime settings array

      protected:
        /// @brief Placement-new a runtime node in the node vector and return a pointer to it
        /// @param nodePtrs: Pointers to the allocated node memory, by index
        /// @param options: TODO: Optionally only initialize ptrs (avoid new)
        template <typename T>
        T* CreateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, InitOptions options) const
        {
            // TODO: Handle options

            T* pNode = static_cast<T*>(nodePtrs[m_nodeIndex]);
            aln::PlacementNew<T>(pNode);

            pNode->m_pSettings = this;

            return pNode;
        }

        /// @brief Set a node based on a given index, only if the index was set
        void SetOptionalNodePtrFromIndex(const std::vector<RuntimeGraphNode*>& nodePtrs, const NodeIndex nodeIndex, RuntimeGraphNode* pNode) const
        {
            if (nodeIndex == InvalidIndex)
                return;

            pNode = nodePtrs[nodeIndex];
        }

        /// @brief Set a node based on a given index
        void SetNodePtrFromIndex(const std::vector<RuntimeGraphNode*>& nodePtrs, const NodeIndex nodeIndex, RuntimeGraphNode* pNode) const
        {
            assert(nodeIndex != InvalidIndex);
            pNode = nodePtrs[nodeIndex];
        }

      public:
        /// @brief Instanciate a node and all its data. Override in derived nodes
        virtual void InstanciateNode(const std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const = 0;
        NodeIndex GetNodeIndex() const { return m_nodeIndex; }
    };

  private:
    const Settings* m_pSettings = nullptr;

  protected:
    // TODO: Status is protected for now as we need to set it in pose nodes's initialize methods... It shouldnt
    Status m_status = Status::Uninitialized;

    // TODO: Pointer to the runtime graph DEFINITION, which holds the settings values
    template <typename T>
    const typename T::Settings* GetSettings() const
    {
        // TODO: Runtime nodes do not contain data, and instead point to the memory where the "graph definition" lies
        return static_cast<const T::Settings*>(m_pSettings);
    }

    virtual void InitializeInternal(GraphContext& context) = 0; // ?
    virtual void ShutdownInternal() = 0;

    virtual NodeValueType GetValueType() const { return NodeValueType::Unknown; }; // ?

  public:
    NodeIndex GetNodeIndex() const { return m_pSettings->GetNodeIndex(); }
    bool IsInitialized() const { return m_status == Status::Initialized; }
    bool IsNodeActive(GraphContext& context) const
    {
        // TODO
        return false;
    }

    virtual void Initialize(GraphContext& context)
    {
        InitializeInternal(context);
        m_status = Status::Initialized;
    }

    virtual void Shutdown()
    {
        ShutdownInternal();
        m_status = Status::Uninitialized;
    }
};

} // namespace aln