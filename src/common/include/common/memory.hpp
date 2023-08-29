#pragma once

#include <assert.h>
#include <cstdlib>
#include <malloc.h>
#include <memory>

namespace aln
{

void* Allocate(size_t size, size_t alignment = 8);
void Free(void* ptr);

template <typename T, typename... ConstructorParameters>
T* New(ConstructorParameters&&... params)
{
    void* ptr = Allocate(sizeof(T), alignof(T));
    return std::construct_at<T>((T*) ptr, std::forward<ConstructorParameters>(params)...);
}

template <typename T, typename... ConstructorParameters>
T* PlacementNew(void* ptr, ConstructorParameters&&... params)
{
    return std::construct_at<T>((T*) ptr, std::forward<ConstructorParameters>(params)...);
}

template <typename T>
void Delete(T*& ptr) noexcept
{
    assert(ptr != nullptr);
    ptr->~T();
    Free(ptr);
}

} // namespace aln