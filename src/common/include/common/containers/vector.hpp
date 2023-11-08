#pragma once

#include <EASTL/vector.h>
#include <EASTL/algorithm.h>

namespace aln
{
template <typename T>
using Vector = eastl::vector<T>;

template <typename T>
Vector<T>::const_iterator VectorFind(const Vector<T>& vector, const T& element) { return eastl::find(vector.begin(), vector.end(), element); }

template<typename T>
bool VectorContains(const Vector<T>& vector, const T& element) { return VectorFind(vector, element) != vector.end(); }
}