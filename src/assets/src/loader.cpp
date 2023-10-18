#include "loader.hpp"

#include "request.hpp"

namespace aln
{
vk::CommandBuffer* IAssetLoader::RequestContext::GetTransferCommandBuffer() { return m_pSourceRequest->GetTransferCommandBuffer(); }
vk::CommandBuffer* IAssetLoader::RequestContext::GetGraphicsCommandBuffer() { return m_pSourceRequest->GetGraphicsCommandBuffer(); }
}