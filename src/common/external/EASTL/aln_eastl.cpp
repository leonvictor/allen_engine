#include "memory.hpp"

#include <EASTL/allocator.h>

// https://github.com/electronicarts/EASTL/blob/master/doc/FAQ.md#cont8-how-do-i-assign-a-custom-allocator-to-an-eastl-container
namespace eastl
{
/// Defines a static default allocator which is constant across all types.
allocator StaticDefaultAllocator;

allocator* GetDefaultAllocator() { return &StaticDefaultAllocator; }
allocator* SetDefaultAllocator(allocator* pAllocator) { return &StaticDefaultAllocator; }
} // namespace eastl

namespace eastl
{
allocator::allocator(const char* EASTL_NAME(pName)) {}
allocator::allocator(const allocator& EASTL_NAME(alloc)) {}
allocator::allocator(const allocator&, const char* EASTL_NAME(pName)) {}
allocator& allocator::operator=(const allocator& EASTL_NAME(alloc)) { return *this; }
const char* allocator::get_name() const { return EASTL_ALLOCATOR_DEFAULT_NAME; }
void allocator::set_name(const char* EASTL_NAME(pName)) {}
void* allocator::allocate(size_t n, int flags) { return aln::Allocate(n, EASTL_ALLOCATOR_MIN_ALIGNMENT); }
void* allocator::allocate(size_t n, size_t alignment, size_t offset, int flags) { return aln::Allocate(n, alignment); }
void allocator::deallocate(void* p, size_t) { aln::Free(p); }
bool operator==(const allocator&, const allocator&) { return true; }
bool operator!=(const allocator&, const allocator&) { return false; }

} // namespace eastl
