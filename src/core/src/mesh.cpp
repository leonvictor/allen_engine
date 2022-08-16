#include "mesh.hpp"

#include <graphics/ubo.hpp>
#include <utils/files.hpp>

namespace aln
{

void Mesh::Bind(vk::CommandBuffer& cb, vk::DeviceSize offset) const
{
    cb.bindVertexBuffers(0, m_vertexBuffer.GetVkBuffer(), offset);
    cb.bindIndexBuffer(m_indexBuffer.GetVkBuffer(), offset, vk::IndexType::eUint32);
    cb.drawIndexed(m_indices.size(), 1, 0, 0, 0);
}
} // namespace aln