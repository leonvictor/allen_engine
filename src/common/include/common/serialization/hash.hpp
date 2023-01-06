#pragma once

#include <xxhash.h>

#include <cctype>
#include <string>

namespace aln
{
namespace hash
{
constexpr static uint32_t Seed = 'ALNH';
}
inline static uint32_t Hash32(const std::string& str) { return XXH32(str.c_str(), str.size(), hash::Seed); }
inline static uint32_t Hash32(const char* str) { return XXH32(str, strlen(str), hash::Seed); }
/// ...

} // namespace aln