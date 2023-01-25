#pragma once

#include "asset_type_id.hpp"
#include <common/serialization/hash.hpp>
#include <reflection/type_info.hpp>

namespace aln
{
/// @brief Assets are identified by their path on disk and their type.
/// @todo Internally use a uint identifier to avoid costly string comparisons
class AssetID
{
  public:
    friend struct ArchiveAccess;

    template <class Archive>
    void Serialize(Archive& archive) const
    {
        archive << m_path;
    }

    template <class Archive>
    void Deserialize(Archive& archive)
    {
        archive >> m_path;
        m_typeID = m_path.substr(m_path.size() - 4);
    }

  private:
    // TODO: change to a hash ?
    std::string m_path;
    AssetTypeID m_typeID;

  public:
    AssetID() = default;
    AssetID(std::string assetPath) : m_path(assetPath),
                                     m_typeID(assetPath.substr(assetPath.size() - 4)) {}

    AssetID(const AssetID& other) : m_path(other.m_path), m_typeID(other.m_typeID) {}

    inline const AssetTypeID& GetAssetTypeID() const { return m_typeID; }
    inline const std::string& GetAssetPath() const { return m_path; }

    /// @brief Returns a string containing only the asset file name (for display purpose only)
    /// @todo Disable out of editor modes ?
    inline const std::string GetAssetName() const
    {
        assert(IsValid());
        auto start = m_path.find_last_of("/\\") + 1;
        auto end = m_path.find_last_of('.') - start;
        return m_path.substr(start, end);
    }

    bool operator==(const AssetID& other) const { return m_path == other.m_path; }
    bool operator!=(const AssetID& other) const { return m_path != other.m_path; }
    bool operator<(const AssetID& other) const { return m_path < other.m_path; }
    operator uint32_t() const { return aln::Hash32(m_path); }

    bool IsValid() const { return m_typeID.IsValid() && !m_path.empty(); }
};

ALN_REGISTER_PRIMITIVE(AssetID);

} // namespace aln