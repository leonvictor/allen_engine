#pragma once

#include "assets/asset_id.hpp"

namespace aln::assets::converter
{
/// @brief Intermediate asset data representation, built by the converter(s), then serialized.
// Serialized raw assets can be deserialized directly into the runtime formats
class IRawAsset
{
  public:
    // TODO: m_id should be set when serializing occur (and be private)
    AssetID m_id;

    virtual void Serialize(BinaryMemoryArchive& archive) = 0;

    const AssetID& GetID() const { return m_id; }
};
} // namespace aln