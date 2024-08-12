#include "services/settings_registry_service.hpp"

namespace aln
{

void SettingsRegistryService::RegisterSettings(ISettings* pSettings)
{
    assert(pSettings != nullptr);
    m_settings.push_back(pSettings);
}

void SettingsRegistryService::UnregisterSettings(ISettings* pSettings)
{
    assert(pSettings != nullptr);
    m_settings.erase_first_unsorted(pSettings);
}

} // namespace aln