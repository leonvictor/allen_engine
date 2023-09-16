#pragma once

#include <EASTL/span.h>

namespace aln
{
template <typename T, eastl_size_t Extent = eastl::dynamic_extent>
using Span = eastl::span<T, Extent>;
}