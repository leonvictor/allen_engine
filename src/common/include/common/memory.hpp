#pragma once

#include <cassert>
#include <cstdlib>
#include <memory>

#ifdef ALN_DEBUG
#include <tracy/Tracy.hpp>
#endif

namespace aln
{

inline void* Allocate(size_t size, size_t alignment = 8)
{

#ifdef _WIN32
    auto ptr = _aligned_malloc(size, alignment);
#else
    auto ptr = std::aligned_alloc(alignment, size);
#endif

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

#ifdef _WIN32
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

#ifdef ALN_DEBUG
// Simple version with no marking to be used by external libraries
inline void* AllocateUnmarked(size_t size, size_t alignment = 8)
{
#ifdef _WIN32
    return _aligned_malloc(size, alignment);
#else
    return std::aligned_alloc(alignment, size);
#endif
}

inline void FreeUnmarked(void* ptr)
{
#ifdef _WIN32
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
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