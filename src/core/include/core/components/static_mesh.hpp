#pragma once

#include "mesh.hpp"

namespace aln
{

/// @brief The StaticMeshComponent component holds a mesh, its material, and the vulkan objects
// representing them on the GPU.
class StaticMeshComponent : public MeshComponent
{
    ALN_REGISTER_TYPE();
};
} // namespace aln