#pragma once

#include <entities/spatial_component.hpp>

namespace aln
{
namespace vkg
{
class Device;
struct UniformBufferObject;
} // namespace vkg

/// @brief The MeshComponent component holds a mesh, its material, and the vulkan objects
// representing them on the GPU.
class MeshComponent : public SpatialComponent
{
    ALN_REGISTER_TYPE()

    friend class GraphicsSystem;

  protected:
    MeshComponent() = default;

    // -------------------------------------------------
    // Components Methods
    // -------------------------------------------------

    virtual void Initialize() override;
    virtual void Shutdown() override;

    virtual bool UpdateLoadingStatus() override = 0;
};
} // namespace aln