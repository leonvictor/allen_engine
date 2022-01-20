#include "converter.hpp"

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

        if (file.path().extension() == ".png" || file.path().extension() == ".jpg" || file.path().extension() == ".TGA")
        {
            exportPath.replace_extension(".tx");

            std::cout << "Texture found, saving to " << exportPath << std::endl;
            ConvertImage(file.path(), exportPath);
        }

        if (file.path().extension() == ".obj")
        {
            std::cout << "found a mesh" << std::endl;
            // TODO: Ignore mesh file prefix
            Assimp::Importer importer;
            const aiScene* pScene = importer.ReadFile(file.path().string(), aiProcess_OptimizeMeshes | aiProcess_GenNormals | aiProcess_FlipUVs); //aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_GenBoundingBoxes);

            ExtractAssimpMeshes(pScene, file.path(), exportPath.parent_path(), config);
        }
        // if (file.path().extension() == ".gltf")
        // {
        //     using namespace tinygltf;
        //     Model model;
        //     TinyGLTF loader;
        //     std::string err;
        //     std::string warn;

        //     bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, file.path().string().c_str());

        //     if (!warn.empty())
        //     {
        //         printf("Warn: %s\n", warn.c_str());
        //     }

        //     if (!err.empty())
        //     {
        //         printf("Err: %s\n", err.c_str());
        //     }

        //     if (!ret)
        //     {
        //         printf("Failed to parse glTF\n");
        //         return -1;
        //     }
        //     else
        //     {
        //         auto folder = exportPath.parent_path() / (file.path().stem().string() + "_GLTF");
        //         fs::create_directory(folder);

        //         extract_gltf_meshes(model, file.path(), folder, convstate);

        //         extract_gltf_materials(model, file.path(), folder, convstate);

        //         extract_gltf_nodes(model, file.path(), folder, convstate);
        //     }
        // }

        if (file.path().extension() == ".fbx")
        {
            Assimp::Importer importer;
            const aiScene* pScene = importer.ReadFile(file.path().string(), aiProcess_OptimizeMeshes | aiProcess_GenNormals | aiProcess_FlipUVs); //aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_GenBoundingBoxes);

            auto folder = exportPath.parent_path() / file.path().stem();
            fs::create_directory(folder);

            ExtractAssimpMaterials(pScene, file.path(), folder, config);
            ExtractAssimpMeshes(pScene, file.path(), folder, config);
            ExtractAssimpNodes(pScene, file.path(), folder, config);

            // std::vector<aiMaterial*> materials;
            // std::vector<std::string> materialNames;
            // materials.reserve(pScene->mNumMaterials);
            // materialNames.reserve(pScene->mNumMaterials);
            // for (int m = 0; m < pScene->mNumMaterials; m++)
            // {
            //     materials.push_back(pScene->mMaterials[m]);
            //     materialNames.push_back(pScene->mMaterials[m]->GetName().C_Str());
            // }

            std::cout << importer.GetErrorString();

            //std::cout << "Assimp Meshes: " << pScene->mMeshes;
        }
    }

    // aln::assets::AssetFile test;
    // auto loaded = aln::assets::LoadBinaryFile("assets/models/assets_export/robot.pfb", test);
    // if (loaded)
    // {
    //     auto info = aln::assets::ReadPrefabInfo(&test);
    // }
    // else
    //     {
    //         std::cout << "Invalid path: " << argv[1];
    //         return -1;
    //     }
    // }
    return 0;
}