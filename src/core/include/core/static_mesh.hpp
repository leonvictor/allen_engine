#pragma once

#include "mesh.hpp"

#include <common/transform.hpp>

namespace aln
{
class StaticMesh : public Mesh
{
    ALN_REGISTER_ASSET_TYPE("mesh")
};

ALN_REGISTER_PRIMITIVE(StaticMesh);
} // namespace aln