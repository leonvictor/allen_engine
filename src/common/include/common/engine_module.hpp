#pragma once

namespace aln
{
class TypeRegistryService;

struct EngineModuleContext
{
    TypeRegistryService* m_pTypeRegistryService;
};

class IEngineModule
{
    virtual void Initialize(EngineModuleContext& context) = 0;
    virtual void Shutdown(EngineModuleContext& context) = 0;
};
} // namespace aln