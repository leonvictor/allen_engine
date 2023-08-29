#pragma once

#include <common/engine_module.hpp>

#include <reflection/services/type_registry_service.hpp>
#include <reflection/type_info.hpp>

namespace aln::Anim
{
class Module : public IEngineModule
{
  public:
    void Initialize(EngineModuleContext& context) override;
};
} // namespace aln::Anim