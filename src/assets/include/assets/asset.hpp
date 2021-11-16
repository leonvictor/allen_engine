#pragma once

#include <set>
#include <string>
#include <typeindex>
#include <typeinfo>

namespace aln
{
// TODO: change to a hash
using AssetGUID = std::string;

class IAsset;

template <class T>
concept AssetType = std::is_base_of<IAsset, T>::value;

/// @brief Abstract base class for all assets (textures, meshes, animation...).
class IAsset
{
    friend class AssetLoader;
    friend class AssetManager;

  private:
    AssetGUID m_id;

  protected:
    enum class Status
    {
        Unloaded,   // Object created and attributes set
        Loaded,     // Asset loaded in memory
        Initialized // Asset loaded and initialization step done
    };

    struct Dependency
    {
        std::type_index type;
        AssetGUID id;

        bool operator<(const Dependency& other) const
        {
            return id < other.id;
        }
    };

    Status m_status = Status::Unloaded;
    std::set<Dependency> m_dependencies;

    IAsset(AssetGUID& guid) : m_id(guid) {}

    template <AssetType T>
    void AddDependency(const AssetGUID& guid)
    {
        m_dependencies.emplace(std::type_index(typeid(T)), guid);
    }

    void RemoveDependency(const AssetGUID& guid)
    {
        std::erase_if(m_dependencies, [&](auto& dep)
            { return dep.id == guid; });
    }

  public:
    const AssetGUID& GetID() const { return m_id; }
    bool IsUnloaded() const { return m_status == Status::Unloaded; }
    bool IsLoaded() const { return m_status == Status::Loaded; }
    bool IsInitialized() const { return m_status == Status::Initialized; }
};
} // namespace aln