#pragma once

#include <assert.h>

#include <common/serialization/binary_archive.hpp>
#include <reflection/type_info.hpp>

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

  public:
    class Settings
    {
        ALN_REGISTER_ABSTRACT_TYPE();

        friend class AnimationGraphCompilationContext;

      private:
        uint32_t m_nodeIndex = InvalidIndex; // Index of the node in the runtime settings array

      protected:
        /// @brief Placement-new a runtime node in the node vector and return a pointer to it
        /// @param nodePtrs: Pointers to the allocated node memory, by index
        /// @param options: TODO: Optionally only initialize ptrs (avoid new)
        template <typename T>
        T* CreateNode(/* todo: const */ std::vector<RuntimeGraphNode*>& nodePtrs, InitOptions options) const
        {
            // TODO: What do we do with the options ?
            // TODO: The vector should be pre-allocated. Placement-new a node of type T in there !
            // TODO: Make sure we allocated the right amount of memory for this node instance here.
            // This should be done when we compile the runtime graph

            // The version we want:
            // T* pNode = static_cast<T*> nodePtrs[m_nodeIndex];
            // new (pNode) T();

            // Temporary solution /!\ with a memory leak as this is never freed !
            auto pNode = aln::New<T>();
            nodePtrs[m_nodeIndex] = pNode;

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

      public:
        /// @brief Instanciate a node and all its data. Override in derived nodes
        virtual void InstanciateNode(/* todo: const */ std::vector<RuntimeGraphNode*>& nodePtrs, AnimationGraphDataset const* pDataSet, InitOptions options) const = 0;

        virtual void Serialize(BinaryMemoryArchive& archive) const
        {
            assert(archive.IsWriting());
            archive << m_nodeIndex;
        }

        virtual void Deserialize(BinaryMemoryArchive& archive)
        {
            assert(archive.IsReading());
            archive >> m_nodeIndex;
        };
    };

  private:
    const Settings* m_pSettings = nullptr;
    NodeIndex m_index = InvalidIndex;

  protected:
    // TODO: Pointer to the runtime graph DEFINITION, which holds the settings values
    template <typename T>
    const typename T::Settings* GetSettings() const
    {
        // TODO: Runtime nodes do not contain data, and instead point to the memory where the "graph definition" lies
        return static_cast<const T::Settings*>(m_pSettings);
    }

    bool IsNodeActive(GraphContext& context) const
    {
        // TODO
        return false;
    }

    virtual void Initialize(GraphContext& context) = 0;                            // ?
    virtual void InitializeInternal(GraphContext& context) = 0;                    // ?
    virtual NodeValueType GetValueType() const { return NodeValueType::Unknown; }; // ?

  public:
    NodeIndex GetIndex() const { return m_index; }
};

} // namespace aln