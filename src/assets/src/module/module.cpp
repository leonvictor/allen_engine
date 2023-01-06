#include "module/module.hpp"

namespace aln::Assets
{
void Module::RegisterTypes(TypeRegistryService* pTypeRegistryService)
{
    pTypeRegistryService->PollRegisteredTypes();
}

} // namespace aln::Assets