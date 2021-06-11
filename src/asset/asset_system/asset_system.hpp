#pragma once

#include <string>
#include <vector>

namespace assets
{
struct AssetFile
{
    char type[4];     // Asset type (MESH, TEXT...)
    uint32_t version; // Format version
    std::string json; //
    std::vector<char> binaryBlob;
};

enum class CompressionMode : uint32_t
{
    None
};

CompressionMode ParseCompressionMode(std::string compressionMode);

bool SaveBinaryFile(std::string path, const AssetFile& file);
bool LoadBinaryFile(std::string path, AssetFile& outputFile);
} // namespace assets