#pragma once

#include <xxhash.h>

#include <cstdint>
#include <cstring>
#include <string>

namespace aln
{
namespace hash
{
static constexpr uint32_t Seed = 1234;
}
inline static uint32_t Hash32(const std::string& str) { return XXH32(str.c_str(), str.size(), hash::Seed); }
inline static uint32_t Hash32(const char* str) { return XXH32(str, strlen(str), hash::Seed); }
/// ...

} // namespace aln