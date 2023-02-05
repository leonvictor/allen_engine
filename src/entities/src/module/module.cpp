#include "module/module.hpp"

namespace aln::Entities
{
void Module::RegisterTypes(TypeRegistryService* pTypeRegistryService)
{
    pTypeRegistryService->PollRegisteredTypes();
}

} // namespace aln::Tooling