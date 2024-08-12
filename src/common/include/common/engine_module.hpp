#pragma once

namespace aln
{
class TypeRegistryService;
class SettingsRegistryService;

struct EngineModuleContext
{
    TypeRegistryService* m_pTypeRegistryService = nullptr;
    SettingsRegistryService* m_pSettingsRegistryService = nullptr;
};

class IEngineModule
{
    virtual void Initialize(EngineModuleContext& context) = 0;
    virtual void Shutdown(EngineModuleContext& context) = 0;
};
} // namespace aln