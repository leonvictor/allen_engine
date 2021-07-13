#include "asset_converter/converter.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

// TODO: Split in 2 different libs
#include <asset_system/asset_system.hpp>
#include <asset_system/texture_asset.hpp>

#include <iostream>

namespace fs = std::filesystem;
using namespace assets;

bool ConvertImage(const fs::path& input, const fs::path& output)
{
    int width, height, channels;
    stbi_uc* pixels = stbi_load(input.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels)
    {
        // TODO: Do not throw but rather log an error
        throw std::runtime_error("Failed to load image at " + input.string());
        return false;
    }

    uint32_t size = width * height * channels;

    TextureInfo info =
        {
            .size = size,
            .format = TextureFormat::RGBA8,
            .pixelSize = {(uint32_t) width, (uint32_t) height, (uint32_t) channels},
            .originalFile = input.string(),
        };

    AssetFile image = PackTexture(&info, pixels);
    stbi_image_free(pixels);
    SaveBinaryFile(output.string(), image);
    return true;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "You need to put the path to the info file";
        return -1;
    }
    else
    {

        fs::path path{argv[1]};

        fs::path directory = path;

        fs::path exported_dir = path.parent_path() / "assets_export";

        std::cout << "loaded asset directory at " << directory << std::endl;

        // ConverterState convstate;
        // convstate.asset_path = path;
        // convstate.export_path = exported_dir;

        for (auto& p : fs::recursive_directory_iterator(directory))
        {
            std::cout << "File: " << p.path() << std::endl;

            auto relative = p.path().lexically_proximate(directory);

            auto export_path = exported_dir / relative;

            if (!fs::is_directory(export_path.parent_path()))
            {
                fs::create_directory(export_path.parent_path());
            }

            if (p.path().extension() == ".png" || p.path().extension() == ".jpg" || p.path().extension() == ".TGA")
            {
                std::cout << "found a texture" << std::endl;

                auto newpath = p.path();
                export_path.replace_extension(".tx");
                ConvertImage(p.path(), export_path);
            }
            //if (p.path().extension() == ".obj") {
            //	std::cout << "found a mesh" << std::endl;
            //
            //	export_path.replace_extension(".mesh");
            //	convert_mesh(p.path(), export_path);
            //}
            // if (p.path().extension() == ".gltf")
            // {
            //     using namespace tinygltf;
            //     Model model;
            //     TinyGLTF loader;
            //     std::string err;
            //     std::string warn;

            //     bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, p.path().string().c_str());

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
            //         auto folder = export_path.parent_path() / (p.path().stem().string() + "_GLTF");
            //         fs::create_directory(folder);

            //         extract_gltf_meshes(model, p.path(), folder, convstate);

            //         extract_gltf_materials(model, p.path(), folder, convstate);

            //         extract_gltf_nodes(model, p.path(), folder, convstate);
            //     }
            // }
            // if (false)
            // { //p.path().extension() == ".fbx") {
            //     const aiScene* scene;
            //     {
            //         Assimp::Importer importer;
            //         //ZoneScopedNC("Assimp load", tracy::Color::Magenta);
            //         //const char* path = p.path().string().c_str();
            //         auto start1 = std::chrono::system_clock::now();
            //         scene = importer.ReadFile(p.path().string(), aiProcess_OptimizeMeshes | aiProcess_GenNormals | aiProcess_FlipUVs); //aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_FlipUVs | aiProcess_GenNormals | aiProcess_GenBoundingBoxes);
            //         auto end = std::chrono::system_clock::now();
            //         auto elapsed = end - start1;
            //         std::cout << "Assimp load time " << elapsed.count() << '\n';
            //         auto folder = export_path.parent_path() / (p.path().stem().string() + "_GLTF");
            //         fs::create_directory(folder);
            //         extract_assimp_materials(scene, p.path(), folder, convstate);
            //         extract_assimp_meshes(scene, p.path(), folder, convstate);
            //         extract_assimp_nodes(scene, p.path(), folder, convstate);

            //         std::vector<aiMaterial*> materials;
            //         std::vector<std::string> materialNames;
            //         materials.reserve(scene->mNumMaterials);
            //         for (int m = 0; m < scene->mNumMaterials; m++)
            //         {
            //             materials.push_back(scene->mMaterials[m]);
            //             materialNames.push_back(scene->mMaterials[m]->GetName().C_Str());
            //         }

            //         std::cout << importer.GetErrorString();

            //         //std::cout << "Assimp Meshes: " << scene->mMeshes;
            //     }
            // }
        }

        //else
        {
            std::cout << "Invalid path: " << argv[1];
            return -1;
        }
    }
}