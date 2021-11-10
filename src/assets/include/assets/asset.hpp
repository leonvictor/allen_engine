#pragma once

#include <string>

namespace aln
{
// TODO: change to a hash
using AssetGUID = std::string;

/// @brief Abstract base class for all assets (textures, meshes, animation...).
class IAsset
{
    friend class AssetLoader;

  private:
    AssetGUID m_id;

  protected:
    enum class Status
    {
        Unloaded,   // Object created and attributes set
        Loaded,     // Asset loaded in memory
        Initialized // Asset loaded and initialization step done
    };

    Status m_status = Status::Unloaded;
    IAsset(AssetGUID& guid) : m_id(guid) {}

  public:
    const AssetGUID& GetID() const { return m_id; }
    bool IsUnloaded() const { return m_status == Status::Unloaded; }
    bool IsLoaded() const { return m_status == Status::Loaded; }
    bool IsInitialized() const { return m_status == Status::Initialized; }
};

template <class T>
concept AssetType = std::is_base_of<IAsset, T>::value;
} // namespace aln