#include "assimp_scene_context.hpp"

namespace aln::assets::converter
{
Matrix4x4 AssimpSceneContext::ToMatrix(const aiMatrix4x4& from)
{
    // Row-major -> Column-major
    Matrix4x4 matrix;
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            matrix[y][x] = from[x][y];
        }
    }
    return matrix;
}

Transform AssimpSceneContext::DecomposeMatrix(const aiMatrix4x4& in)
{
    aiVector3D scaling;
    aiQuaternion rotation;
    aiVector3D position;
    in.Decompose(scaling, rotation, position);

    return Transform(
        ToVec3(position),
        ToQuaternion(rotation),
        ToVec3(scaling));
}

Transform AssimpSceneContext::RevertSceneTransform(const Transform& transform) const
{
    auto reverted = m_inverseSceneTransform * transform;
    return reverted;
}

Vec3 AssimpSceneContext::RevertSceneTransform(const Vec3& vector) const
{
    auto reverted = m_inverseSceneTransform.TransformPoint(vector);
    return reverted;
}

bool AssimpSceneContext::TryGetSkeleton(const std::string& skeletonName, RawSkeleton*& pOutSkeleton) const
{
    auto [it, emplaced] = m_skeletons.try_emplace(skeletonName);
    pOutSkeleton = &(it->second);
    return !emplaced;
}

Transform AssimpSceneContext::GetGlobalTransform(const aiNode* pNode) const
{
    struct NodeLookUp
    {
        const AssimpSceneContext* m_pSceneContext;

        const aiNode* m_pNode;
        Transform m_globalTransform;

        void FindNode(const aiNode* pNode, const aiMatrix4x4& parentGlobalTransform)
        {
            auto nodeLocalTransform = pNode->mTransformation;
            auto nodeGlobalTransform = nodeLocalTransform * parentGlobalTransform;

            if (pNode == m_pNode)
            {
                m_globalTransform = m_pSceneContext->DecomposeMatrix(nodeGlobalTransform);
                return;
            }

            for (auto childIndex = 0; childIndex < pNode->mNumChildren; ++childIndex)
            {
                FindNode(pNode->mChildren[childIndex], nodeGlobalTransform);
            }
        }

        NodeLookUp(const AssimpSceneContext* pSceneContext, const aiNode* pNode) : m_pSceneContext(pSceneContext), m_pNode(pNode)
        {
            const auto pRootNode = m_pSceneContext->m_pScene->mRootNode;
            const static auto identity = aiMatrix4x4(aiVector3D(1.0f, 1.0f, 1.0f), aiQuaternion(1.0f, 0.0f, 0.0f, 0.0f), aiVector3D(0, 0, 0));
            FindNode(pRootNode, identity);
        }
    };

    auto lookup = NodeLookUp(this, pNode);
    return lookup.m_globalTransform;
}
} // namespace aln::assets::converter