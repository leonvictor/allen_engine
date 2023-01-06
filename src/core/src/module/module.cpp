#include "module/module.hpp"

namespace aln::Core
{
void Module::RegisterTypes(TypeRegistryService* pTypeRegistryService)
{
    pTypeRegistryService->PollRegisteredTypes();
}

} // namespace aln::Core