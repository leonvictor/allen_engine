#include "module/module.hpp"

// TODO: We use tooling as a namespace bc Editor is taken (it would be better suited here tho)
namespace aln::Tooling
{
void Module::Initialize(EngineModuleContext& context)
{
    context.m_pTypeRegistryService->PollRegisteredTypes();
}

} // namespace aln::Tooling