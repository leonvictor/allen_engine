#pragma once

#include <cstdlib>
#include <memory>

namespace aln
{

template <typename T, typename... ConstructorParameters>
T* New(ConstructorParameters&&... params)
{
    void* ptr = malloc(sizeof(T));

#ifdef ALN_ENABLE_TRACING
    TracyAlloc(ptr, count);
#endif

    return std::construct_at<T>((T*) ptr, std::forward<ConstructorParameters>(params)...);
}

template <typename T>
void Delete(T*& ptr) noexcept
{
    assert(T != nullptr);

#ifdef ALN_ENABLE_TRACING
    TracyFree(ptr);
#endif

    ptr->~T();
    free(ptr);
}
} // namespace aln