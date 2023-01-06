#pragma once

#include <common/engine_module.hpp>

#include <reflection/services/type_registry_service.hpp>
#include <reflection/type_info.hpp>

namespace aln::Tooling
{
class Module : public IEngineModule
{
  public:
    void RegisterTypes(TypeRegistryService* pTypeRegistryService) override;
};
} // namespace aln::Tooling