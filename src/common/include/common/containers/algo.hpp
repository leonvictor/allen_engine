#pragma once

#include <EASTL/algorithm.h>

namespace aln::Algo
{

template <typename Container>
void Reverse(Container& container) { eastl::reverse(container.begin(), container.end()); }

template <typename Container, typename T = decltype(Container::value_type)>
T::iterator LowerBound(const Container& container, const T& value)
{
    return eastl::lower_bound(container.begin(), container.end(), value);
}

template <typename Container, typename T>
uint32_t LowerBoundIndex(const Container& container, const T& value)
{
    auto it = eastl::lower_bound(container.begin(), container.end(), value);
    return eastl::distance(container.begin(), it);
}
} // namespace aln::Algo