#pragma once

#include <set>
#include <string>
#include <typeindex>
#include <typeinfo>

#include <array>
#include <assert.h>
#include <cstring>
#include <filesystem>
#include <string>

namespace aln
{

/// @brief Identifies asset types as a four character string.
/// This unique name is also used as the extension for asset files.
struct AssetTypeID
{

  private:
    std::array<char, 4> m_id;

  public:
    AssetTypeID() = default;

    AssetTypeID(const std::string& str)
    {
        assert(str.size() == 4);
        std::copy(str.begin(), str.end(), m_id.data());
    }

    AssetTypeID(const char* str)
    {
        assert(strlen(str) == 4);
        m_id[0] = str[0];
        m_id[1] = str[1];
        m_id[2] = str[2];
        m_id[3] = str[3];
    }

    bool operator==(const AssetTypeID& other) { return m_id == other.m_id; }
    bool operator!=(const AssetTypeID& other) { return m_id != other.m_id; }
};

class AssetID
{
  private:
    // TODO: change to a hash
    std::string m_path;
    AssetTypeID m_typeID;

  public:
    // Infer asset type from file extension
    AssetID(std::string assetPath) : m_path(assetPath),
                                     m_typeID(assetPath.substr(assetPath.size() - 4)) {}

    AssetID(const AssetID& other) : m_path(other.m_path), m_typeID(other.m_typeID) {}

    inline const AssetTypeID& GetAssetTypeID() const { return m_typeID; }
    inline const std::string& GetAssetPath() const { return m_path; }

    bool operator==(const AssetID& other) const { return m_path == other.m_path; }
    bool operator!=(const AssetID& other) const { return m_path != other.m_path; }
    bool operator<(const AssetID& other) const { return m_path < other.m_path; }
};

class IAsset;

template <class T>
concept AssetType = std::is_base_of<IAsset, T>::value;

/// @brief Abstract base class for all assets (textures, meshes, animation...).
class IAsset
{
    friend class AssetLoader;
    friend class AssetManager;

  private:
    AssetID m_id;

  protected:
    enum class Status
    {
        Unloaded, // Object created and attributes set
        Loaded,   // Asset loaded in memory
    };

    struct Dependency
    {
        // TODO: type_index should no longer be necessary since AssetID contains an AssetTypeID
        std::type_index type;
        AssetID id;

        bool operator<(const Dependency& other) const
        {
            return id < other.id;
        }
    };

    Status m_status = Status::Unloaded;
    std::set<Dependency> m_dependencies;

    IAsset(AssetID& guid) : m_id(guid) {}

    template <AssetType T>
    void AddDependency(const AssetID& guid)
    {
        m_dependencies.emplace(std::type_index(typeid(T)), guid);
    }

    void RemoveDependency(const AssetID& guid)
    {
        std::erase_if(m_dependencies, [&](auto& dep)
            { return dep.id == guid; });
    }

  public:
    inline const AssetID& GetID() const { return m_id; }
    inline bool IsUnloaded() const { return m_status == Status::Unloaded; }
    inline bool IsLoaded() const { return m_status == Status::Loaded; }
    inline bool IsInitialized() const { return m_status == Status::Initialized; }
    inline bool operator==(const IAsset& other) const { return m_id == other.m_id; }

    virtual AssetTypeID GetAssetTypeID() const = 0;
};
} // namespace aln

#define ALN_REGISTER_ASSET_TYPE(assetTypeExtension)                                                 \
  public:                                                                                           \
    static AssetTypeID GetStaticAssetTypeID() { return AssetTypeID(assetTypeExtension); }           \
    virtual AssetTypeID GetAssetTypeID() const override { return AssetTypeID(assetTypeExtension); } \
                                                                                                    \
  private: