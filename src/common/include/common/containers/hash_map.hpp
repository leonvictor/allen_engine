#pragma once

#include <EASTL/hash_map.h>

namespace aln
{
template <typename Key, typename Value, typename Hash = eastl::hash<Key>>
using HashMap = eastl::hash_map<Key, Value, Hash>;
}