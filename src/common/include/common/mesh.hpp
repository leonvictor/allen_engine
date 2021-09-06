#pragma once

#include <string>
#include <vector>

#include "vertex.hpp"

namespace aln
{

// TODO: Complete gltf format
// In glTF, meshes are defined as arrays of primitives. Primitives correspond to the data required for GPU draw calls. Primitives specify one or more attributes, corresponding to the vertex attributes used in the draw calls.
struct PrimitiveComponent
{
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t materialIndex;
};

/// @brief Mesh resource
class Mesh
{
    friend class MeshRenderer;

  protected:
    std::string m_sourceFile;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    std::vector<PrimitiveComponent> m_primitives;

  public:
    /// @brief Load the mesh from the source file.
    bool Load();

    /// @brief Free mesh resources.
    void Unload();

    void RevertNormals();
};
} // namespace aln