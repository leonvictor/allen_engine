#include "module/module.hpp"

namespace aln::Assets
{
void Module::Initialize(EngineModuleContext& context)
{
    context.m_pTypeRegistryService->PollRegisteredTypes();
}

void Module::Shutdown(EngineModuleContext& context)
{
}
} // namespace aln::Assets