#pragma once

#include "resources/buffer.hpp"

#include <common/mesh.hpp>
#include <memory>

namespace aln::vkg
{
class Device;
class UniformBufferObject;

/// @brief Mesh backed by GPU buffers.
class DrawMesh : public Mesh
{
  private:
    resources::Buffer m_vertexBuffer;
    resources::Buffer m_indexBuffer;

  public:
    DrawMesh(std::string modelPath);

    /// @brief Create and fill the vulkan buffers to back the mesh.
    void CreateGraphicResources(std::shared_ptr<Device> pDevice);

    /// @brief Reset the vulkan buffers backing the mesh on GPU.
    void FreeGraphicResources();

    void Bind(vk::CommandBuffer& cb, vk::DeviceSize offset);
};
} // namespace aln::vkg