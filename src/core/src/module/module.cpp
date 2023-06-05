#include "module/module.hpp"

namespace aln::Core
{
void Module::Initialize(EngineModuleContext& context)
{
    context.m_pTypeRegistryService->PollRegisteredTypes();
}

} // namespace aln::Core