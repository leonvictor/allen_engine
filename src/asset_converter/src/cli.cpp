#include "asset_converter.hpp"
#include "raw_assets/raw_texture.hpp"

#include <filesystem>
#include <iostream>

using namespace aln::assets::converter;

// TODO: Propagate a shared context so that we can detect when the same asset is used multiple times

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "AssetConverter expects a directory to process as a first argument" << std::endl;
        return -1;
    }

    std::filesystem::path inputDirectory = std::filesystem::path(argv[1]);

    std::cout << "Loaded asset directory: " << inputDirectory << std::endl;

    for (auto& file : std::filesystem::recursive_directory_iterator(inputDirectory))
    {
        std::cout << "File: " << file.path() << std::endl;

        auto exportPath = rootOutputDirectory / file.path().lexically_proximate(inputDirectory);

        // Create subdirectories if necessary
        if (!std::filesystem::is_directory(exportPath.parent_path()))
        {
            std::filesystem::create_directory(exportPath.parent_path());
        }

        auto extension = file.path().extension();
        if (extension == ".png" || extension == ".jpg" || extension == ".TGA")
        {
            exportPath.replace_extension(".text");

            std::cout << "Texture found, saving to " << exportPath << std::endl;
            FileTextureReader::ReadTexture(file.path(), exportPath);
        }

        if (extension == ".fbx" || extension == ".obj" || extension == ".gltf")
        {
            auto folder = exportPath.parent_path() / file.path().stem();
            AssetConverter::Convert(file.path(), folder, aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_Triangulate | aiProcess_GenUVCoords | aiProcess_PopulateArmatureData);
        }
    }

    return 0;
}