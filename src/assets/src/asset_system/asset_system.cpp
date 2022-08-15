#include "asset_system/asset_system.hpp"

#include <assert.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <common/binary_archive.hpp>

namespace aln::assets
{

bool SaveBinaryFile(const std::string& path, const AssetFile& file)
{
    std::cout << "Saving to: " << path << std::endl;

    assert(file.m_dependencies.size() < std::numeric_limits<uint8_t>::max());
    uint8_t dependencyCount = file.m_dependencies.size();

    auto archive = BinaryArchive(std::filesystem::path(path), BinaryArchive::Mode::Write);

    archive << file.m_version;
    archive << (uint32_t) file.m_assetTypeID;

    archive << dependencyCount;
    for (auto& dependency : file.m_dependencies)
    {
        auto& assetPath = dependency.GetAssetPath();
        archive << assetPath;
    }

    archive << file.m_metadata;
    archive << file.m_binary;

    return true;
}

bool LoadBinaryFile(const std::string& path, AssetFile& outputFile)
{
    auto archive = BinaryArchive(path, BinaryArchive::Mode::Read);

    archive >> outputFile.m_version;

    // TODO: Stream directly to m_assetTypeID.m_id
    uint32_t id;
    archive >> id;
    outputFile.m_assetTypeID = AssetTypeID(id);

    uint8_t dependencyCount;
    archive >> dependencyCount;
    std::string assetPath;
    for (auto idx = 0; idx < dependencyCount; idx++)
    {
        archive >> assetPath;
        outputFile.m_dependencies.push_back(AssetID(assetPath));
    }

    archive >> outputFile.m_metadata;
    archive >> outputFile.m_binary;

    return true;
}
} // namespace aln::assets