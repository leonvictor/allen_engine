#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/gtx/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <common/transform.hpp>

#include <assert.h>
#include <filesystem>
#include <map>

#include "raw_assets/raw_skeleton.hpp"

namespace aln::assets::converter
{

class AssimpSceneContext
{
    friend class AssetConverter;

  private:
    const aiScene* m_pScene = nullptr;

    Transform m_inverseSceneTransform = Transform::Identity;

    mutable std::map<std::string, RawSkeleton> m_skeletons;
    std::vector<AssetID> m_materials;

    std::filesystem::path m_sourceFilePath;
    std::filesystem::path m_outputDirectoryPath;

  public:
    const aiScene* GetScene() const { return m_pScene; }
    const aiNode* GetRootNode() const { return m_pScene->mRootNode; }

    // Helpers translating assimp struct to ours
    /// @todo Rename to more descriptive names (ToVec3, ToTransform)...
    static glm::mat4x4 ToGLM(const aiMatrix4x4& from);
    static glm::vec3 ToGLM(const aiVector3D& in) { return glm::vec3(in.x, in.y, in.z); }
    static glm::quat ToGLM(const aiQuaternion& in) { return glm::quat(in.w, in.x, in.y, in.z); }
    static glm::vec2 ToVec2(const aiVector3D& in) { return glm::vec2{in.x, in.y}; }
    static glm::vec3 ToVec3(const aiColor4D& in) { return glm::vec3{in[0], in[1], in[2]}; }

    /// @brief Decompose an assimp matrix in an aln transform
    static Transform DecomposeMatrix(const aiMatrix4x4& in);

    Transform RevertSceneTransform(const Transform& transform) const;
    glm::vec3 RevertSceneTransform(const glm::vec3& vector) const;

    /// @brief Try to find an existing skeleton with a matching name. If no skeleton is found, a new one is created.
    /// @param pOutSkeleton: a pointer to the existing or newly created skeleton
    /// @return true if the skeleton already existed
    bool TryGetSkeleton(const std::string& skeletonName, RawSkeleton*& pOutSkeleton) const;
    const AssetID& GetMaterial(size_t materialIndex)
    {
        assert(materialIndex >= 0 && materialIndex < m_materials.size());
        return m_materials[materialIndex];
    }

    Transform GetGlobalTransform(const aiNode* pNode) const;

    // -------------
    // Path manipulation
    // -------------

    const std::filesystem::path& GetSourceFile() const { return m_sourceFilePath; }
    const std::filesystem::path& GetOutputDirectory() const { return m_outputDirectoryPath; }
    std::filesystem::path GetPathRelativeToOutput(const std::filesystem::path& path) const
    {
        return path.lexically_proximate(m_outputDirectoryPath);
    }
};
} // namespace aln::assets::converter
