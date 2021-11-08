#pragma once

#include <string>

namespace aln
{
// TODO: change to a hash
using AssetGUID = std::string;

/// @brief Abstract base class for all assets (textures, meshes, animation...).
/// Does not provide much behavior but allows us to restict the user to only using the "right" asset types
/// @warning Most of the implementation is not done ! Only inherit from this for now.
class IAsset
{
  private:
    enum class Status
    {
        Unloaded,   // Object created and attributes set
        Loaded,     // Asset loaded in memory
        Initialized // Asset loaded and initialization step done
    };

    Status m_status = Status::Unloaded; // TODO: Unused
    AssetGUID m_id;                     // TODO: Let the asset loader set the ID

  public:
    const AssetGUID& GetID() { return m_id; }
    bool IsUnloaded() { return m_status == Status::Unloaded; }
    bool IsLoaded() { return m_status == Status::Loaded || m_status == Status::Initialized; }
    bool IsInitialized() { return m_status == Status::Initialized; }
};
} // namespace aln