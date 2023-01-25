#pragma once

#include "serialization/hash.hpp"

namespace aln
{
class StringID
{
  private:
    uint32_t m_hash;

  public:
    StringID() = default;
    StringID(const std::string& str) : m_hash(Hash32(str)) {}
    StringID(const char* str) : m_hash(Hash32(str)) {}
    StringID(uint32_t hash) : m_hash(hash) {}

    inline uint32_t GetHash() const { return m_hash; }
    bool IsValid() const { return m_hash != 0; }

    bool operator==(const StringID& id) const { return m_hash == id.m_hash; }
    bool operator!=(const StringID& id) const { return m_hash != id.m_hash; }
    bool operator<(const StringID& id) const { return m_hash < id.m_hash; }

    static StringID InvalidID() { return StringID((uint32_t) 0); };
};

// StringID must be trivial to easily be serialized
static_assert(std::is_trivial_v<StringID>);

} // namespace aln

namespace std
{
template <>
struct hash<aln::StringID>
{
    size_t operator()(const aln::StringID& id) const { return id.GetHash(); }
};
} // namespace std