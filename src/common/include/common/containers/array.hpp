#pragma once

#include <EASTL/array.h>

namespace aln
{
	template<typename T, eastl_size_t N> using Array = eastl::array<T, N>;
}