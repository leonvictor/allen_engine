#include "module/module.hpp"

namespace aln::Assets
{
void Module::Initialize(EngineModuleContext& context)
{
    context.m_pTypeRegistryService->PollRegisteredTypes();
}
} // namespace aln::Assets