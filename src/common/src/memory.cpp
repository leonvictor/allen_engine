#include "memory.hpp"

namespace aln
{
void* Allocate(size_t size, size_t alignment)
{
    /// @todo _aligned_malloc is msvc-only
    auto ptr = _aligned_malloc(size, alignment);
#ifdef ALN_ENABLE_TRACING
    TracyAlloc(ptr, size);
#endif
    return ptr;
}

void Free(void* ptr)
{
#ifdef ALN_ENABLE_TRACING
    TracyFree(ptr);
#endif
    /// @todo _aligned_free is msvc-only
    _aligned_free(ptr);
}
}