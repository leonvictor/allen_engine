#include "module/module.hpp"

namespace aln::Entities
{
void Module::Initialize(EngineModuleContext& context)
{
    context.m_pTypeRegistryService->PollRegisteredTypes();
}

void Module::Shutdown(EngineModuleContext& context)
{
}

} // namespace aln::Tooling