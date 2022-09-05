#pragma once

#include <assert.h>
#include <string>

namespace aln
{
/// @brief Identifies asset types as a four character string.
/// This unique name is also used as the extension for asset files.
class AssetTypeID
{
  private:
    uint32_t m_id;

  public:
    AssetTypeID() = default;
    AssetTypeID(uint32_t id) : m_id(id) {}
    AssetTypeID(const std::string& str)
    {
        assert(str.size() == 4);
        m_id = str[0] | (str[1] << 8) | (str[2] << 16) | (str[3] << 24);
    }

    AssetTypeID(const char* str)
    {
        assert(strlen(str) == 4);
        m_id = str[0] | (str[1] << 8) | (str[2] << 16) | (str[3] << 24);
    }

    std::string ToString() const
    {
        std::string str = "0000";
        str[0] = (char) (m_id);
        str[1] = (char) (m_id >> 8);
        str[2] = (char) (m_id >> 16);
        str[3] = (char) (m_id >> 24);
        return str;
    }

    bool operator==(const AssetTypeID& other) const { return m_id == other.m_id; }
    bool operator!=(const AssetTypeID& other) const { return m_id != other.m_id; }
    bool operator<(const AssetTypeID& other) const { return m_id < other.m_id; }

    operator uint32_t() const { return m_id; }
    operator uint32_t&() { return m_id; }

    bool IsValid() const { return m_id != 0; }
};

// AssetTypeID must be trivial for serialization purposes
/// @note I'm not sure this is good practice. I think compile-time asserts are cool to notice early if something broke
// but I might be missing something
static_assert(std::is_trivial<AssetTypeID>());
} // namespace aln