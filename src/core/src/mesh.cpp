#include "mesh.hpp"

#include <graphics/ubo.hpp>
#include <utils/files.hpp>

namespace aln
{

void Mesh::CreateGraphicResources(const std::shared_ptr<vkg::Device>& pDevice)
{
    assert(IsLoaded());

    // Create vertex buffer
    vkg::resources::Buffer vertexStagingBuffer(pDevice, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_vertices);
    m_vertexBuffer = vkg::resources::Buffer(pDevice, vertexStagingBuffer.GetSize(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Create index buffer
    vkg::resources::Buffer indexStagingBuffer(pDevice, vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, m_indices);
    m_indexBuffer = vkg::resources::Buffer(pDevice, indexStagingBuffer.GetSize(), vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);

    pDevice->GetTransferCommandPool().Execute([&](vk::CommandBuffer cb)
        {
            vertexStagingBuffer.CopyTo(cb, m_vertexBuffer);
            indexStagingBuffer.CopyTo(cb, m_indexBuffer); });
}

void Mesh::FreeGraphicResources()
{
    assert(IsInitialized());

    m_vertexBuffer = vkg::resources::Buffer();
    m_indexBuffer = vkg::resources::Buffer();
}

void Mesh::Bind(vk::CommandBuffer& cb, vk::DeviceSize offset) const
{
    cb.bindVertexBuffers(0, m_vertexBuffer.GetVkBuffer(), offset);
    cb.bindIndexBuffer(m_indexBuffer.GetVkBuffer(), offset, vk::IndexType::eUint32);
    cb.drawIndexed(m_indices.size(), 1, 0, 0, 0);
}
} // namespace aln