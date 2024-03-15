#pragma once

#include <assets/handle.hpp>
#include <core/skeletal_mesh.hpp>
#include <reflection/reflected_type.hpp>
#include <reflection/type_info.hpp>

namespace aln
{
struct PreviewSceneSettings : public reflect::IReflected
{
    ALN_REGISTER_TYPE()

  public:
    AssetHandle<SkeletalMesh> m_pSkeletalMesh;
};
} // namespace aln