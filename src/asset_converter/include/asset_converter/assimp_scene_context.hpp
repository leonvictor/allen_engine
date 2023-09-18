#pragma once

#include "raw_assets/raw_skeleton.hpp"

#include <common/maths/quaternion.hpp>
#include <common/maths/matrix4x4.hpp>
#include <common/maths/vec2.hpp>
#include <common/maths/vec3.hpp>
#include <common/transform.hpp>
#include <common/containers/hash_map.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assert.h>
#include <filesystem>

namespace aln::assets::converter
{

class AssimpSceneContext
{
    friend class AssetConverter;

  private:
    const aiScene* m_pScene = nullptr;

    Transform m_inverseSceneTransform = Transform::Identity;

    mutable HashMap<std::string, RawSkeleton, std::hash<std::string>> m_skeletons;
    Vector<AssetID> m_materials;

    std::filesystem::path m_sourceFilePath;
    std::filesystem::path m_outputDirectoryPath;

  public:
    const aiScene* GetScene() const { return m_pScene; }
    const aiNode* GetRootNode() const { return m_pScene->mRootNode; }

    // Helpers translating assimp struct to ours
    /// @todo Rename to more descriptive names (ToVec3, ToTransform)...
    static Matrix4x4 ToGLM(const aiMatrix4x4& from);
    static Vec3 ToGLM(const aiVector3D& in) { return Vec3(in.x, in.y, in.z); }
    static Quaternion ToGLM(const aiQuaternion& in) { return Quaternion(in.w, in.x, in.y, in.z); }
    static Vec2 ToVec2(const aiVector3D& in) { return Vec2{in.x, in.y}; }
    static Vec3 ToVec3(const aiColor4D& in) { return Vec3{in[0], in[1], in[2]}; }

    /// @brief Decompose an assimp matrix in an aln transform
    static Transform DecomposeMatrix(const aiMatrix4x4& in);

    Transform RevertSceneTransform(const Transform& transform) const;
    Vec3 RevertSceneTransform(const Vec3& vector) const;

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
