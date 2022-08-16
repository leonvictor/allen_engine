#pragma once

#include <assert.h>
#include <cstdlib>
#include <memory>

#include <aln_common_export.h>

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
    assert(ptr != nullptr);

#ifdef ALN_ENABLE_TRACING
    TracyFree(ptr);
#endif

    ptr->~T();
    free((void*) ptr);
}
} // namespace aln