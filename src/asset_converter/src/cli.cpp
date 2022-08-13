#include "asset_converter.hpp"
#include "image_converter.hpp"

#include <iostream>

using namespace aln::assets::converter;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "AssetConverter expects a directory to process as a first argument" << std::endl;
        return -1;
    }

    ConverterConfig config{fs::path(argv[1])}; // TODO: output arg

    std::cout << "Loaded asset directory: " << config.inputDirectory << std::endl;

    for (auto& file : fs::recursive_directory_iterator(config.inputDirectory))
    {
        std::cout << "File: " << file.path() << std::endl;

        auto exportPath = config.rootOutputDirectory / file.path().lexically_proximate(config.inputDirectory);

        // Create subdirectories if necessary
        if (!fs::is_directory(exportPath.parent_path()))
        {
            fs::create_directory(exportPath.parent_path());
        }

        auto extension = file.path().extension();
        if (extension == ".png" || extension == ".jpg" || extension == ".TGA")
        {
            exportPath.replace_extension(".tx");

            std::cout << "Texture found, saving to " << exportPath << std::endl;
            ConvertImage(file.path(), exportPath);
        }

        if (extension == ".fbx" || extension == ".obj" || extension == ".gltf")
        {
            auto folder = exportPath.parent_path() / file.path().stem();
            AssetConverter::Convert(file.path(), folder, aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_Triangulate | aiProcess_GenUVCoords | aiProcess_PopulateArmatureData);
        }
    }

    return 0;
}