#include "converter.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/vec3.hpp>

#include <assets/asset_system/asset_system.hpp>
#include <assets/asset_system/material_asset.hpp>
#include <assets/asset_system/mesh_asset.hpp>
#include <assets/asset_system/prefab_asset.hpp>
#include <assets/asset_system/texture_asset.hpp>

#include <iostream>

namespace aln::assets::converter
{
namespace fs = std::filesystem;

fs::path ConverterConfig::RelativeToOutput(const fs::path& path) const
{
    return path.lexically_proximate(rootOutputDirectory);
}

std::string GetAssimpMaterialName(const aiScene* pScene, int materialIndex)
{
    return "MAT_" + std::to_string(materialIndex) + "_" + std::string{pScene->mMaterials[materialIndex]->GetName().C_Str()};
}

std::string GetAssimpMeshName(const aiScene* pScene, int meshIndex)
{
    return "MESH_" + std::to_string(meshIndex) + "_" + std::string{pScene->mMeshes[meshIndex]->mName.C_Str()};
}

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
/// @brief Extract materials from a assimp scene and save them in asset binary format
/// @param: pScene: pointer to the assimp scene
/// @param: input: ?
/// @param: folder: ?
void ExtractAssimpMaterials(const aiScene* pScene, const fs::path& input, const fs::path& outputFolder, const ConverterConfig& config)
{
    for (int m = 0; m < pScene->mNumMaterials; m++)
    {
        aiMaterial* pMaterial = pScene->mMaterials[m];

        MaterialInfo materialInfo;
        materialInfo.baseEffect = "defaultPBR";
        materialInfo.transparency = TransparencyMode::Transparent;

        for (int p = 0; p < pMaterial->mNumProperties; p++)
        {
            aiMaterialProperty* pt = pMaterial->mProperties[p];
            switch (pt->mType)
            {
            case aiPTI_String:
            {
                const char* data = pt->mData;
                materialInfo.customProperties[pt->mKey.C_Str()] = data;
            }
            break;
            case aiPTI_Float:
            {
                std::stringstream ss;
                ss << *(float*) pt->mData;
                materialInfo.customProperties[pt->mKey.C_Str()] = ss.str();

                if (strcmp(pt->mKey.C_Str(), "$mat.opacity") == 0)
                {
                    float num = *(float*) pt->mData;
                    if (num != 1.0)
                    {
                        materialInfo.transparency = TransparencyMode::Transparent;
                    }
                }
            }
            break;
            }
        }

        // Check opacity
        std::string texPath = "";
        if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE))
        {
            aiString assimppath;
            pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &assimppath);

            fs::path texturePath = &assimppath.data[0];
            // Unreal compat
            texturePath = texturePath.filename();
            texPath = "T_" + texturePath.string();
        }
        else if (pMaterial->GetTextureCount(aiTextureType_BASE_COLOR))
        {
            aiString assimppath;
            pMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &assimppath);

            fs::path texturePath = &assimppath.data[0];
            // Unreal compat
            texturePath = texturePath.filename();
            texPath = "T_" + texturePath.string();
        }

        // Force a default texture
        else
        {
            texPath = "Default";
        }

        fs::path baseColorPath = outputFolder.parent_path() / texPath;
        baseColorPath.replace_extension(".tx");
        baseColorPath = config.RelativeToOutput(baseColorPath);
        materialInfo.textures["baseColor"] = baseColorPath.string();

        AssetFile newFile = PackMaterial(&materialInfo);

        // Save to disk
        std::string materialName = GetAssimpMaterialName(pScene, m);
        fs::path materialPath = outputFolder / (materialName + ".mat");
        SaveBinaryFile(materialPath.string().c_str(), newFile);
    }
}

void ExtractAssimpMeshes(const aiScene* pScene, const fs::path& input, const fs::path& output, const ConverterConfig& config)
{
    for (int meshindex = 0; meshindex < pScene->mNumMeshes; meshindex++)
    {

        auto pMesh = pScene->mMeshes[meshindex];

        // using VertexFormat = assets::Vertex_f32_PNCV;
        // auto VertexFormatEnum = assets::VertexFormat::PNCV_F32;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        std::string meshName = GetAssimpMeshName(pScene, meshindex);

        vertices.reserve(pMesh->mNumVertices);
        for (int v = 0; v < pMesh->mNumVertices; v++)
        {
            Vertex vert;
            vert.pos = {
                pMesh->mVertices[v].x,
                pMesh->mVertices[v].y,
                pMesh->mVertices[v].z};

            vert.normal[0] = pMesh->mNormals[v].x;
            vert.normal[1] = pMesh->mNormals[v].y;
            vert.normal[2] = pMesh->mNormals[v].z;

            if (pMesh->GetNumUVChannels() >= 1)
            {
                vert.texCoord[0] = pMesh->mTextureCoords[0][v].x;
                vert.texCoord[1] = pMesh->mTextureCoords[0][v].y;
            }
            else
            {
                vert.texCoord[0] = 0;
                vert.texCoord[1] = 0;
            }
            if (pMesh->HasVertexColors(0))
            {
                vert.color[0] = pMesh->mColors[0][v].r;
                vert.color[1] = pMesh->mColors[0][v].g;
                vert.color[2] = pMesh->mColors[0][v].b;
            }
            else
            {
                vert.color[0] = 1;
                vert.color[1] = 1;
                vert.color[2] = 1;
            }

            // vertices[v] = vert;
            vertices.push_back(vert);
        }
        indices.reserve(pMesh->mNumFaces * 3);
        for (int f = 0; f < pMesh->mNumFaces; f++)
        {
            auto& faceIndices = pMesh->mFaces[f].mIndices;
            indices.push_back(faceIndices[0]);
            indices.push_back(faceIndices[1]);
            indices.push_back(faceIndices[2]);
            // indices[f * 3 + 0] = pMesh->mFaces[f].mIndices[0];
            // indices[f * 3 + 1] = pMesh->mFaces[f].mIndices[1];
            // indices[f * 3 + 2] = pMesh->mFaces[f].mIndices[2];

            // TODO: test this
            //assimp fbx creates bad normals, just regen them
            // if (true)
            // {
            //     int v0 = indices[f * 3 + 0];
            //     int v1 = indices[f * 3 + 1];
            //     int v2 = indices[f * 3 + 2];

            //     glm::vec3 normal = glm::normalize(glm::cross(
            //         vertices[v2].pos - vertices[v0].pos,
            //         vertices[v1].pos - vertices[v0].pos));

            //     memcpy(&vertices[v0].normal, &normal, sizeof(float) * 3);
            //     memcpy(&vertices[v1].normal, &normal, sizeof(float) * 3);
            //     memcpy(&vertices[v2].normal, &normal, sizeof(float) * 3);
            // }
        }

        MeshInfo meshinfo;
        // meshinfo.vertexFormat = VertexFormatEnum;
        meshinfo.vertexBufferSize = vertices.size() * sizeof(Vertex);
        meshinfo.indexBufferSize = indices.size() * sizeof(uint32_t);
        meshinfo.indexSize = sizeof(uint32_t);
        meshinfo.originalFile = input.string();

        meshinfo.bounds = assets::CalculateBounds(vertices.data(), vertices.size());

        assets::AssetFile newFile = assets::PackMesh(&meshinfo, (char*) vertices.data(), (char*) indices.data());

        fs::path meshpath = output / (meshName + ".mesh");

        // Save to disk
        SaveBinaryFile(meshpath.string().c_str(), newFile);
    }
}

void ExtractAssimpMatrix(aiMatrix4x4& in, std::array<float, 16>& out)
{
    glm::mat4 matrix;
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            matrix[y][x] = in[x][y];
        }
    }
    memcpy(&matrix, &out, sizeof(glm::mat4));
}

void ExtractAssimpNodes(const aiScene* pScene, const fs::path& input, const fs::path& outputFolder, const ConverterConfig& config)
{
    PrefabInfo prefabInfo;

    std::array<float, 16> identityMatrix;
    {
        glm::mat4 ident{1.f};
        memcpy(&identityMatrix, &ident, sizeof(glm::mat4));
    }

    uint64_t lastNode = 0;

    std::function<void(aiNode*, uint64_t)> processNode = [&](aiNode* pNode, uint64_t parentID)
    {
        uint64_t nodeindex = lastNode;
        lastNode++;

        // Extract node's parent (if any)
        if (parentID != nodeindex)
        {
            prefabInfo.nodeParents[nodeindex] = parentID;
        }

        // Extract the node's transform
        std::array<float, 16> transform;
        ExtractAssimpMatrix(pNode->mTransformation, transform);

        prefabInfo.nodeTransforms[nodeindex] = prefabInfo.transforms.size();
        prefabInfo.transforms.push_back(transform);

        // Extract node name
        std::string nodeName = pNode->mName.C_Str();
        if (nodeName.size() > 0)
        {
            prefabInfo.nodeNames[nodeindex] = nodeName;
        }

        // Extract the node's meshes
        for (size_t msh = 0; msh < pNode->mNumMeshes; ++msh)
        {
            int meshIndex = pNode->mMeshes[msh];
            auto materialName = GetAssimpMaterialName(pScene, pScene->mMeshes[meshIndex]->mMaterialIndex);
            auto meshName = GetAssimpMeshName(pScene, meshIndex);

            fs::path materialpath = outputFolder / (materialName + ".mat");
            fs::path meshpath = outputFolder / (meshName + ".mesh");

            PrefabInfo::NodeMesh nodeMesh;
            nodeMesh.meshPath = config.RelativeToOutput(meshpath).string();
            nodeMesh.materialPath = config.RelativeToOutput(materialpath).string();

            prefabInfo.nodeMeshes[lastNode] = nodeMesh;
            prefabInfo.nodeParents[lastNode] = nodeindex;

            prefabInfo.nodeTransforms[lastNode] = prefabInfo.transforms.size();
            prefabInfo.transforms.push_back(identityMatrix);

            lastNode++;
        }

        for (size_t ch = 0; ch < pNode->mNumChildren; ++ch)
        {
            processNode(pNode->mChildren[ch], nodeindex);
        }
    };

    // aiMatrix4x4 mat{};
    // glm::mat4 rootmat{1};

    // for (int y = 0; y < 4; y++)
    // {
    //     for (int x = 0; x < 4; x++)
    //     {
    //         mat[x][y] = rootmat[y][x];
    //     }
    // }

    processNode(pScene->mRootNode, 0);

    AssetFile file = PackPrefab(prefabInfo);

    fs::path scenefilepath = (outputFolder.parent_path()) / input.stem();
    scenefilepath.replace_extension(".pfb");

    // Save to disk
    SaveBinaryFile(scenefilepath.string(), file);
}
} // namespace aln::assets::converter