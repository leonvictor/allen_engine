#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/vec3.hpp>

#include <assets/asset_system/animation_clip_asset.hpp>
#include <assets/asset_system/asset_system.hpp>
#include <assets/asset_system/material_asset.hpp>
#include <assets/asset_system/mesh_asset.hpp>
#include <assets/asset_system/prefab_asset.hpp>
#include <assets/asset_system/texture_asset.hpp>

#include <filesystem>
#include <iostream>

namespace aln::assets::converter
{
// TODO: Only in cpp
namespace fs = std::filesystem;
/// @brief TODO
class AssetConverter
{
  private:
    Assimp::Importer m_importer;

    const aiScene* m_pScene;
    std::filesystem::path m_inputFile;
    std::filesystem::path m_outputDirectory;

    AssetConverter(std::filesystem::path inputFile, std::filesystem::path outputDirectory, int postProcessFlags)
        : m_inputFile(inputFile),
          m_outputDirectory(outputDirectory),
          m_pScene(m_importer.ReadFile(inputFile.string(), postProcessFlags))
    {
        fs::create_directory(outputDirectory);

        ExtractMaterials();
        ExtractMeshes();
        ExtractAnimations();
        ExtractNodes();

        std::cout << m_importer.GetErrorString();
    }

    fs::path GetPathRelativeToOutput(const fs::path& path) const
    {
        return path.lexically_proximate(m_outputDirectory);
    }

    /// @brief Extract materials from a assimp scene and save them in asset binary format
    void ExtractMaterials()
    {
        for (int materialIndex = 0; materialIndex < m_pScene->mNumMaterials; materialIndex++)
        {
            aiMaterial* pMaterial = m_pScene->mMaterials[materialIndex];

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

            fs::path baseColorPath = m_outputDirectory.parent_path() / texPath;
            baseColorPath.replace_extension(".tx");
            baseColorPath = GetPathRelativeToOutput(baseColorPath);
            materialInfo.textures["baseColor"] = baseColorPath.string();

            AssetFile newFile = PackMaterial(&materialInfo);

            // Save to disk
            std::string materialName = std::string(pMaterial->GetName().C_Str()) + "_" + std::to_string(materialIndex);
            fs::path materialPath = m_outputDirectory / (materialName + ".mat");
            SaveBinaryFile(materialPath.string(), newFile);
        }
    }

    fs::path GetMaterialAssetPath(uint32_t materialIndex)
    {
        auto pMaterial = m_pScene->mMaterials[materialIndex];
        std::string materialName = std::string(pMaterial->GetName().C_Str()) + "_" + std::to_string(materialIndex);
        fs::path materialPath = m_outputDirectory / (materialName + ".mat");
        return materialPath;
    }

    fs::path GetMeshAssetPath(uint32_t meshIndex)
    {
        auto pMesh = m_pScene->mMeshes[meshIndex];
        std::string meshName = std::string(pMesh->mName.C_Str()) + "_" + std::to_string(meshIndex);
        fs::path meshPath = m_outputDirectory / (meshName + ".mesh");
        return meshPath;
    }

    void ProcessMesh(const aiMesh* pMesh, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
    {
        vertices.reserve(pMesh->mNumVertices);
        for (int vertexIndex = 0; vertexIndex < pMesh->mNumVertices; vertexIndex++)
        {
            Vertex vert;
            vert.pos = {
                pMesh->mVertices[vertexIndex].x,
                pMesh->mVertices[vertexIndex].y,
                pMesh->mVertices[vertexIndex].z};

            vert.normal[0] = pMesh->mNormals[vertexIndex].x;
            vert.normal[1] = pMesh->mNormals[vertexIndex].y;
            vert.normal[2] = pMesh->mNormals[vertexIndex].z;

            if (pMesh->GetNumUVChannels() >= 1)
            {
                vert.texCoord[0] = pMesh->mTextureCoords[0][vertexIndex].x;
                vert.texCoord[1] = pMesh->mTextureCoords[0][vertexIndex].y;
            }
            else
            {
                vert.texCoord[0] = 0;
                vert.texCoord[1] = 0;
            }

            if (pMesh->HasVertexColors(0))
            {
                vert.color[0] = pMesh->mColors[0][vertexIndex].r;
                vert.color[1] = pMesh->mColors[0][vertexIndex].g;
                vert.color[2] = pMesh->mColors[0][vertexIndex].b;
            }
            else
            {
                vert.color[0] = 1;
                vert.color[1] = 1;
                vert.color[2] = 1;
            }

            // vertices[vertexIndex] = vert;
            vertices.push_back(vert);
        }

        indices.reserve(pMesh->mNumFaces * 3);
        for (int faceIndex = 0; faceIndex < pMesh->mNumFaces; faceIndex++)
        {
            auto& faceIndices = pMesh->mFaces[faceIndex].mIndices;
            indices.push_back(faceIndices[0]);
            indices.push_back(faceIndices[1]);
            indices.push_back(faceIndices[2]);

            // TODO: test this
            //assimp fbx creates bad normals, just regen them
            if (true)
            {
                int v0 = indices[faceIndex * 3 + 0];
                int v1 = indices[faceIndex * 3 + 1];
                int v2 = indices[faceIndex * 3 + 2];

                glm::vec3 normal = glm::normalize(glm::cross(
                    vertices[v2].pos - vertices[v0].pos,
                    vertices[v1].pos - vertices[v0].pos));

                memcpy(&vertices[v0].normal, &normal, sizeof(float) * 3);
                memcpy(&vertices[v1].normal, &normal, sizeof(float) * 3);
                memcpy(&vertices[v2].normal, &normal, sizeof(float) * 3);
            }
        }
    }

    void ExtractMeshes()
    {
        // TODO: Put all submeshes from the same node in the same file
        for (int meshIndex = 0; meshIndex < m_pScene->mNumMeshes; ++meshIndex)
        {
            auto pMesh = m_pScene->mMeshes[meshIndex];

            // TODO: Extract Skeleton
            std::cout << pMesh->mNumBones << std::endl;
            if (pMesh->mNumBones > 0)
            {
                std::cout << std::endl;
                std::cout << "Mesh w/ " << pMesh->mNumBones << " bones" << std::endl;
                auto pArmature = pMesh->mBones[0]->mArmature;
            }
            for (size_t boneIndex = 0; boneIndex < pMesh->mNumBones; ++boneIndex)
            {
                auto pBone = pMesh->mBones[boneIndex];
                auto pArmature = pBone->mArmature;
                // std::cout << pBone->mName.C_Str() << std::endl;
            }

            // using VertexFormat = assets::Vertex_f32_PNCV;
            // auto VertexFormatEnum = assets::VertexFormat::PNCV_F32;

            // std::string meshName = GetAssimpMeshName(m_pScene, meshIndex);

            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            ProcessMesh(pMesh, vertices, indices);

            MeshInfo meshinfo;
            // meshinfo.vertexFormat = VertexFormatEnum;
            meshinfo.vertexBufferSize = vertices.size() * sizeof(Vertex);
            meshinfo.indexBufferSize = indices.size() * sizeof(uint32_t);
            meshinfo.indexTypeSize = sizeof(uint32_t);
            meshinfo.originalFile = m_inputFile.string();
            meshinfo.bounds = CalculateBounds(vertices.data(), vertices.size());

            AssetFile file = PackMesh(&meshinfo, (char*) vertices.data(), (char*) indices.data());

            std::string meshName = std::string(pMesh->mName.C_Str()) + "_" + std::to_string(meshIndex);
            fs::path meshpath = m_outputDirectory / (meshName + ".mesh");

            // Save to disk
            SaveBinaryFile(meshpath.string(), file);
        }
    }

    void ExtractAnimations()
    {
        for (size_t animIndex = 0; animIndex < m_pScene->mNumAnimations; ++animIndex)
        {
            aiAnimation* pAnim = m_pScene->mAnimations[animIndex];

            AnimationClipInfo animInfo;
            animInfo.originalFile = m_inputFile.string();
            animInfo.name = pAnim->mName.C_Str();
            animInfo.duration = pAnim->mDuration;
            animInfo.framesPerSecond = pAnim->mTicksPerSecond;

            // TODO: Handle Assimp MeshChannels
            // TODO: Handle Assimp MorphChannels

            std::vector<float> animData;
            animInfo.tracks.reserve(pAnim->mNumChannels);
            for (size_t channelIndex = 0; channelIndex < pAnim->mNumChannels; ++channelIndex)
            {
                auto pChannel = pAnim->mChannels[channelIndex];

                // Assume keys are synchronized
                // TODO: don't
                auto numKeys = pChannel->mNumRotationKeys;
                assert(numKeys == pChannel->mNumPositionKeys &&
                       numKeys == pChannel->mNumScalingKeys
                    //    &&numKeys == pAnim->mDuration
                );

                TrackInfo trackInfo;
                trackInfo.boneName = pChannel->mNodeName.C_Str();
                trackInfo.numKeys = numKeys;

                // BoneTrack track;
                // track.boneName = pChannel->mNodeName.C_Str();
                // track.frames.reserve(numKeys);

                animData.reserve(animData.size() + (numKeys * (1 + 4 + 3 + 3)));
                for (size_t keyIndex = 0; keyIndex < numKeys; keyIndex++)
                {
                    // TODO: Key time might vary by channel
                    animData.push_back((float) pChannel->mPositionKeys[keyIndex].mTime);

                    auto translation = pChannel->mPositionKeys[keyIndex].mValue;
                    animData.push_back((float) translation.x);
                    animData.push_back((float) translation.y);
                    animData.push_back((float) translation.z);

                    auto rotation = pChannel->mRotationKeys[keyIndex].mValue;
                    animData.push_back((float) rotation.w);
                    animData.push_back((float) rotation.x);
                    animData.push_back((float) rotation.y);
                    animData.push_back((float) rotation.z);

                    auto scale = pChannel->mScalingKeys[keyIndex].mValue;
                    animData.push_back((float) scale.x);
                    animData.push_back((float) scale.y);
                    animData.push_back((float) scale.z);

                    // TODO: Handle Assimp pre and post state
                }
                animInfo.tracks.push_back(trackInfo);
            }

            AssetFile file = PackAnimationClip(&animInfo, animData);

            std::string animName = pAnim->mName.C_Str();
            std::replace(animName.begin(), animName.end(), '|', '_');
            auto path = (m_outputDirectory / animName);
            path.replace_extension(".anim");

            SaveBinaryFile(path.string(), file);
        }
    }

    /// @brief Convert an assimp matrix to an aln one, and then to a flat float array
    void ConvertMatrix(aiMatrix4x4& in, std::array<float, 16>& out)
    {
        glm::mat4 matrix;
        for (int y = 0; y < 4; y++)
        {
            for (int x = 0; x < 4; x++)
            {
                matrix[y][x] = in[x][y]; // TODO: ?
            }
        }
        memcpy(&matrix, &out, sizeof(glm::mat4));
    }

    void ExtractNodes()
    {
        PrefabInfo prefabInfo;

        // // TODO: Extract the meshes's skeletons
        // std::map<aiNode*, bool> skeletonNecessaryNodes;
        // std::function<void(aiNode*)> populateNodeMap = [&](aiNode* pNode)
        // {
        //     skeletonNecessaryNodes[pNode] = false;
        //     for (size_t childIndex = 0; childIndex < pNode->mNumChildren; ++childIndex)
        //     {
        //         populateNodeMap(pNode->mChildren[childIndex]);
        //     }
        // };

        // populateNodeMap(m_pScene->mRootNode);

        std::array<float, 16> identityMatrix;
        glm::mat4 ident(1.0f);
        memcpy(&identityMatrix, &ident, sizeof(glm::mat4));

        uint64_t lastNode = 0;
        uint8_t depth = 0;
        uint32_t nodeCount = 0;
        std::function<void(aiNode*, uint64_t)> processNode = [&](aiNode* pNode, uint64_t parentID)
        {
            nodeCount++;

            uint64_t nodeindex = lastNode;
            lastNode++;

            for (int i = 0; i < depth; i++)
            {
                std::cout << " ";
            }
            std::cout << pNode->mName.C_Str();
            std::cout << "(" << pNode->mNumMeshes << " meshes)" << std::endl;

            // Get node's parent (if any)
            if (parentID != nodeindex)
            {
                prefabInfo.nodeParents[nodeindex] = parentID;
            }

            // Get the node's transform
            std::array<float, 16> transform;
            ConvertMatrix(pNode->mTransformation, transform);

            prefabInfo.nodeTransforms[nodeindex] = prefabInfo.transforms.size();
            prefabInfo.transforms.push_back(transform);

            // Get node name
            std::string nodeName = pNode->mName.C_Str();
            if (nodeName.size() > 0)
            {
                prefabInfo.nodeNames[nodeindex] = nodeName;
            }

            // Extract the node mesh/material pair
            /// @note Assimp does not handle multiple material per mesh and will instead split
            /// meshes with multiple material
            if (pNode->mNumMeshes > 0)
            {
                for (size_t nodeMeshIndex = 0; nodeMeshIndex < pNode->mNumMeshes; ++nodeMeshIndex)
                {
                    auto meshIndex = pNode->mMeshes[nodeMeshIndex];
                    auto materialIndex = m_pScene->mMeshes[meshIndex]->mMaterialIndex;

                    fs::path materialPath = GetMaterialAssetPath(materialIndex);
                    fs::path meshPath = GetMeshAssetPath(meshIndex);

                    PrefabInfo::NodeMeshInfo nodeMesh;
                    nodeMesh.meshPath = GetPathRelativeToOutput(meshPath).string();
                    nodeMesh.materialPath = GetPathRelativeToOutput(materialPath).string();

                    prefabInfo.nodeMeshes[lastNode] = nodeMesh;
                    prefabInfo.nodeParents[lastNode] = nodeindex;
                    prefabInfo.nodeTransforms[lastNode] = prefabInfo.transforms.size();

                    prefabInfo.transforms.push_back(identityMatrix);

                    lastNode++;
                }
            }
            else
            {
                // TODO: Keep the transformation
            }

            depth++;
            for (size_t ch = 0; ch < pNode->mNumChildren; ++ch)
            {
                processNode(pNode->mChildren[ch], nodeindex);
            }
            depth--;
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

        processNode(m_pScene->mRootNode, 0);
        std::cout << nodeCount << " nodes." << std::endl;
        AssetFile file = PackPrefab(prefabInfo);

        fs::path scenefilepath = (m_outputDirectory.parent_path()) / m_inputFile.stem();
        scenefilepath.replace_extension(".pfb");

        // Save to disk
        SaveBinaryFile(scenefilepath.string(), file);
    }

  public:
    static void Convert(std::filesystem::path inputFile, std::filesystem::path outputDirectory, int postProcessFlags)
    {
        auto converter = AssetConverter(inputFile, outputDirectory, postProcessFlags);
    }
};
} // namespace aln::assets::converter