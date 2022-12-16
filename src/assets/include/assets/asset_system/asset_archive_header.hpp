#pragma once

#include "../asset_id.hpp"
#include "../asset_type_id.hpp"

#include <vector>

namespace aln
{
enum class AssetCompressionMode : uint8_t
{
    None,
    LZ4
};

/// @brief Header for on-disk asset archives.
/// @note The expected format of asset archives is: | Header | Body size | Body (binary) |
class AssetArchiveHeader
{
    friend struct ArchiveAccess;

    static constexpr uint32_t AssetArchiveVersion = 1;

  private:
    // System info
    uint32_t m_version = AssetConverterVersion;

    // Asset info
    AssetTypeID m_assetTypeID;
    std::vector<AssetID> m_dependencies;

    // Compression info
    /// @todo Set when compressing
    AssetCompressionMode m_compressionMode;
    uint32_t m_uncompressedBodySize = 0;

  public:
    AssetArchiveHeader() = default;
    AssetArchiveHeader(const AssetTypeID& typeID) : m_assetTypeID(typeID) {}
    void AddDependency(const AssetID& assetID) { m_dependencies.push_back(assetID); }

    const std::vector<AssetID>& GetDependencies() const { return m_dependencies; }

    // Serialization
    /// @todo Make private when serialization system allows it
    template <class Archive>
    void Serialize(Archive& archive) const
    {
        archive << m_version;
        archive << m_assetTypeID;
        archive << m_dependencies;
        archive << m_compressionMode;
        archive << m_uncompressedBodySize;
    }

    template <class Archive>
    void Deserialize(Archive& archive)
    {
        archive >> m_version;
        archive >> m_assetTypeID;
        archive >> m_dependencies;
        archive >> m_compressionMode;
        archive >> m_uncompressedBodySize;
    }
};
} // namespace aln