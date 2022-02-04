#include "asset_system/asset_system.hpp"

#include <fstream>
#include <iostream>

namespace aln::assets
{

bool SaveBinaryFile(const std::string& path, const AssetFile& file)
{
    uint32_t metadataSize = file.metadata.size();
    uint32_t binarySize = file.binary.size();

    std::ofstream outfile;
    outfile.open(path, std::ios::binary | std::ios::out);

    outfile.write(reinterpret_cast<const char*>(&file.type), 1);                   // First byte represent the type
    outfile.write(reinterpret_cast<const char*>(&file.version), sizeof(uint32_t)); // Then the format version
    outfile.write(reinterpret_cast<const char*>(&metadataSize), sizeof(uint32_t)); // Metadata size
    outfile.write(reinterpret_cast<const char*>(&binarySize), sizeof(uint32_t));   // Binary blob size
    outfile.write(file.metadata.data(), metadataSize);                             // Metadata
    outfile.write(reinterpret_cast<const char*>(file.binary.data()), binarySize);  // Binary blob

    outfile.close();
    return true;
}

bool LoadBinaryFile(const std::string& path, AssetFile& outputFile)
{
    uint32_t metadataSize;
    uint32_t binarySize;

    std::ifstream infile;
    infile.open(path, std::ios::binary);

    if (!infile.is_open())
        return false;

    infile.seekg(0);

    infile.read(reinterpret_cast<char*>(&outputFile.type), 1);                   // Read type
    infile.read(reinterpret_cast<char*>(&outputFile.version), sizeof(uint32_t)); // Read version
    infile.read(reinterpret_cast<char*>(&metadataSize), sizeof(uint32_t));       // Read metadata size
    infile.read(reinterpret_cast<char*>(&binarySize), sizeof(uint32_t));         // Read binary blob size

    outputFile.metadata.resize(metadataSize);
    outputFile.binary.resize(binarySize);

    infile.read(reinterpret_cast<char*>(outputFile.metadata.data()), metadataSize); // Read metadata
    infile.read(reinterpret_cast<char*>(outputFile.binary.data()), binarySize);     // Read binary blob

    return true;
}
} // namespace aln::assets