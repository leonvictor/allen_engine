#include <reflection/reflection.hpp>

#include "anim/animation_clip.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "skeletal_mesh.hpp"
#include "static_mesh.hpp"
#include "texture.hpp"

namespace aln
{
ALN_REGISTER_PRIMITIVE(Mesh);
ALN_REGISTER_PRIMITIVE(SkeletalMesh);
ALN_REGISTER_PRIMITIVE(StaticMesh);
ALN_REGISTER_PRIMITIVE(Texture);
ALN_REGISTER_PRIMITIVE(AnimationClip);
ALN_REGISTER_PRIMITIVE(Material);
} // namespace aln