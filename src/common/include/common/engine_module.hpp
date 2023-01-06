#pragma once

namespace aln
{
class TypeRegistryService;

class IEngineModule
{
    virtual void RegisterTypes(TypeRegistryService*) = 0;
};
} // namespace aln