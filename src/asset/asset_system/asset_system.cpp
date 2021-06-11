#include "asset_system.hpp"

#include <fstream>
#include <iostream>

using namespace assets;

CompressionMode ParseCompressionMode(std::string compressionMode)
{

    if (compressionMode == "none")
    {
        return CompressionMode::None;
    }
}

bool SaveBinaryFile(std::string path, const AssetFile& file)
{
    std::ofstream outfile;
    outfile.open(path, std::ios::binary | std::ios::out);

    // First 4 chars are the type
    outfile.write(file.type, 4);
    // Then the format version
    outfile.write((const char*) &file.version, sizeof(uint32_t));

    // JSON length
    uint32_t jLength = file.json.size();
    outfile.write((const char*) &jLength, sizeof(uint32_t));

    // binary length
    uint32_t bLength = file.binaryBlob.size();
    outfile.write((const char*) &bLength, sizeof(uint32_t));

    // JSON
    outfile.write(file.json.data(), jLength);

    // Binary
    outfile.write(file.binaryBlob.data(), bLength);

    outfile.close();
    return true;
}

bool LoadBinaryFile(std::string path, AssetFile& outputFile)
{
    std::ifstream infile;
    infile.open(path, std::ios::binary);

    if (!infile.is_open())
        return false;

    infile.seekg(0);
    // Read type
    infile.read(outputFile.type, 4);
    // Version
    infile.read((char*) &outputFile.version, sizeof(uint32_t));

    // Json length
    uint32_t jLength;
    infile.read((char*) &jLength, sizeof(uint32_t));

    // Binary length
    uint32_t bLength;
    infile.read((char*) &bLength, sizeof(uint32_t));

    outputFile.json.resize(jLength);
    infile.read((char*) outputFile.json.data(), jLength);

    outputFile.binaryBlob.resize(jLength);
    infile.read((char*) outputFile.binaryBlob.data(), bLength);

    return true;
}
