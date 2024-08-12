#include "module/module.hpp"

#include <common/services/settings_registry_service.hpp>
#include <config/path.h>
#include <reflection/services/type_registry_service.hpp>
#include <reflection/type_info.hpp>

namespace aln::Assets
{
void Module::Initialize(EngineModuleContext& context)
{
    m_settings.m_workingDirectory = std::filesystem::current_path();
    m_settings.m_editorAssetsDirectory = m_settings.m_workingDirectory / "../assets/editor/";

    // TODO: Allow setting custom path when switching projects
    m_settings.m_projectAssetsDirectory = DEFAULT_ASSETS_DIR;

    context.m_pSettingsRegistryService->RegisterSettings(&m_settings);
    context.m_pTypeRegistryService->PollRegisteredTypes();
}

void Module::Shutdown(EngineModuleContext& context)
{
    context.m_pSettingsRegistryService->UnregisterSettings(&m_settings);
}
} // namespace aln::Assets