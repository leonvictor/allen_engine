#include "components/animation_graph_component.hpp"

namespace aln
{
ALN_REGISTER_IMPL_BEGIN(COMPONENTS, AnimationGraphComponent)
ALN_REFLECT_MEMBER(m_pGraphDefinition)
ALN_REFLECT_MEMBER(m_pGraphDataset)
ALN_REFLECT_MEMBER(m_pSkeleton)
ALN_REGISTER_IMPL_END()
}