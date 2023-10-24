#include "loader.hpp"

#include "request.hpp"

namespace aln
{
TransferQueuePersistentCommandBuffer& IAssetLoader::RequestContext::GetTransferCommandBuffer() { return m_pSourceRequest->GetTransferCommandBuffer(); }
GraphicsQueuePersistentCommandBuffer& IAssetLoader::RequestContext::GetGraphicsCommandBuffer() { return m_pSourceRequest->GetGraphicsCommandBuffer(); }
}