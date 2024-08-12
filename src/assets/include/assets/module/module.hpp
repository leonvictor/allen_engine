#pragma once

#include "../assets_settings.hpp"

#include <common/engine_module.hpp>

namespace aln::Assets
{
class Module : public IEngineModule
{
  private:
    AssetsSettings m_settings;

  public:
    void Initialize(EngineModuleContext& context) override;
    void Shutdown(EngineModuleContext& context) override;
};
} // namespace aln::Assets