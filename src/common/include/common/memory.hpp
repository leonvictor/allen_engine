#pragma once

#include <assert.h>
#include <cstdlib>
#include <malloc.h>
#include <memory>

#ifdef ALN_DEBUG
#include <tracy/Tracy.hpp>
#endif

namespace aln
{

inline void* Allocate(size_t size, size_t alignment = 8)
{
    auto ptr = _aligned_malloc(size, alignment);
#ifdef ALN_DEBUG
    TracyAllocS(ptr, size, 15);
#endif
    return ptr;
}

inline void Free(void* ptr)
{
#ifdef ALN_DEBUG
    TracyFreeS(ptr, 15);
#endif
    _aligned_free(ptr);
}

#ifdef ALN_DEBUG
// Simple version with no marking to be used by external libraries
inline void* AllocateUnmarked(size_t size, size_t alignment = 8)
{
    return _aligned_malloc(size, alignment);
}

inline void FreeUnmarked(void* ptr)
{
    _aligned_free(ptr);
}
#endif

template <typename T, typename... ConstructorParameters>
inline T* New(ConstructorParameters&&... params)
{
    void* ptr = Allocate(sizeof(T), alignof(T));
    return std::construct_at<T>((T*) ptr, std::forward<ConstructorParameters>(params)...);
}

template <typename T, typename... ConstructorParameters>
inline T* PlacementNew(void* ptr, ConstructorParameters&&... params)
{
    return std::construct_at<T>((T*) ptr, std::forward<ConstructorParameters>(params)...);
}

template <typename T>
inline void Delete(T*& ptr) noexcept
{
    assert(ptr != nullptr);
    ptr->~T();
    Free(ptr);
}
} // namespace aln