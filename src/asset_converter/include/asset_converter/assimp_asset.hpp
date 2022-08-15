#pragma once

#include <filesystem>

#include <assets/asset_id.hpp>

namespace aln
{
/// @brief Base class for assimp assets to be serialized
class AssimpAsset
{
  protected:
    AssetID m_id;

  public:
    const AssetID& GetAssetID() const { return m_id; }
    const AssetTypeID& GetAssetTypeID() const { return m_id.GetAssetTypeID(); }
    const std::string& GetAssetPath() const { return m_id.GetAssetPath(); }

    /// @note: It might be cool to have the path / id generation in common in this base class
    // virtual void Serialize(const std::filesystem::path& path) = 0; // TODO
};
} // namespace aln