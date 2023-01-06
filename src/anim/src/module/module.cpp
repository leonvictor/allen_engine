#include "module/module.hpp"

namespace aln::Anim
{
void Module::RegisterTypes(TypeRegistryService* pTypeRegistryService)
{
    pTypeRegistryService->PollRegisteredTypes();
}

} // namespace aln::Anim