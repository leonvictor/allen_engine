#include "assimp_scene_context.hpp"

namespace aln::assets::converter
{
AssimpSceneContext::AssimpSceneContext(const std::filesystem::path& inputFile, const std::filesystem::path& outputDirectory)
    : m_sourceFilePath(inputFile),
      m_outputDirectoryPath(outputDirectory),
      m_pScene(m_importer.ReadFile(inputFile.string(), AssimpPostProcessFlags))
{
    if (m_pScene == nullptr)
    {
        // TODO: Handle error
        auto errorString = m_importer.GetErrorString();
        assert(false);
    }

    m_inverseSceneTransform = DecomposeMatrix(m_pScene->mRootNode->mTransformation).GetInverse();
}

glm::mat4x4 AssimpSceneContext::ToGLM(const aiMatrix4x4& from)
{
    // Row-major -> Column-major
    /// @todo Pick one
    glm::mat4 matrix;
    for (int y = 0; y < 4; y++)
    {
        for (int x = 0; x < 4; x++)
        {
            matrix[y][x] = from[x][y];
        }
    }

    glm::mat4 to;
    to[0][0] = from.a1;
    to[1][0] = from.a2;
    to[2][0] = from.a3;
    to[3][0] = from.a4;
    to[0][1] = from.b1;
    to[1][1] = from.b2;
    to[2][1] = from.b3;
    to[3][1] = from.b4;
    to[0][2] = from.c1;
    to[1][2] = from.c2;
    to[2][2] = from.c3;
    to[3][2] = from.c4;
    to[0][3] = from.d1;
    to[1][3] = from.d2;
    to[2][3] = from.d3;
    to[3][3] = from.d4;

    assert(matrix == to);

    return to;
}

Transform AssimpSceneContext::DecomposeMatrix(const aiMatrix4x4& in)
{
    // auto matrix = ToGLM(in);
    // return Transform(matrix);

    aiVector3D scaling;
    aiQuaternion rotation;
    aiVector3D position;
    in.Decompose(scaling, rotation, position);

    // TEST:
    auto tQuat = glm::quat(1, 0.3, 0.2, 1);
    auto tQuatAi = aiQuaternion(1, 0.3, 0.2, 1);

    auto glmRot = ToGLM(rotation);

    auto rotatedAi = rotation * tQuatAi;
    auto rotatedGlm = glmRot * tQuat;

    // ----------------
    return Transform(
        ToGLM(position),
        ToGLM(rotation),
        ToGLM(scaling));
}

Transform AssimpSceneContext::RevertSceneTransform(const Transform& transform) const
{
    auto reverted = m_inverseSceneTransform * transform;
    return reverted;
}

glm::vec3 AssimpSceneContext::RevertSceneTransform(const glm::vec3& vector) const
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
            auto identity = aiMatrix4x4(aiVector3D(1, 1, 1), aiQuaternion(1, 0, 0, 0), aiVector3D(0, 0, 0));
            FindNode(pRootNode, identity);
        }
    };

    auto lookup = NodeLookUp(this, pNode);
    return lookup.m_globalTransform;
}
} // namespace aln::assets::converter