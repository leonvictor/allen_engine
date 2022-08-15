#pragma once

#include "asset_type_id.hpp"

namespace aln
{
/// @brief Assets are identified by their path on disk and their type.
/// @todo Internally use a uint identifier to avoid costly string comparisons
class AssetID
{
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

    bool operator==(const AssetID& other) const { return m_path == other.m_path; }
    bool operator!=(const AssetID& other) const { return m_path != other.m_path; }
    bool operator<(const AssetID& other) const { return m_path < other.m_path; }

    bool IsValid() const { return m_typeID.IsValid() && !m_path.empty(); }
};
} // namespace aln