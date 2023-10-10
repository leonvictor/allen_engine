#include "module/module.hpp"

namespace aln::Anim
{
void Module::Initialize(EngineModuleContext& context)
{
    context.m_pTypeRegistryService->PollRegisteredTypes();
}

void Module::Shutdown(EngineModuleContext& context)
{
}
} // namespace aln::Anim