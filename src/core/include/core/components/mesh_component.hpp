#pragma once

#include <entities/spatial_component.hpp>

namespace aln
{
namespace vkg
{
class RenderEngine;
struct UniformBufferObject;
} // namespace vkg

/// @brief The MeshComponent component holds a mesh, its material, and the vulkan objects
// representing them on the GPU.
class MeshComponent : public SpatialComponent
{
    ALN_REGISTER_TYPE()

    friend class WorldRenderingSystem;

  protected:
    MeshComponent() = default;

    // -------------------------------------------------
    // Components Methods
    // -------------------------------------------------

    void Initialize() override;
    void Shutdown() override;

    bool UpdateLoadingStatus() override = 0;
};
} // namespace aln