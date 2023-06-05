#include "module/module.hpp"

namespace aln::Anim
{
void Module::Initialize(EngineModuleContext& context)
{
    context.m_pTypeRegistryService->PollRegisteredTypes();
}

} // namespace aln::Anim