#pragma once

#include <memory>
#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <graphics/resources/buffer.hpp>
#include <graphics/resources/image.hpp>
#include <graphics/ubo.hpp>

#include <entities/spatial_component.hpp>

#include "../material.hpp"
#include "../mesh.hpp"
#include "../texture.hpp"

#include <assets/asset_service.hpp>
#include <assets/handle.hpp>

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
    friend class GraphicsSystem;

  protected:
    vkg::Device* m_pDevice;

  public:
    virtual void SetMesh(const std::string& path) = 0;

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