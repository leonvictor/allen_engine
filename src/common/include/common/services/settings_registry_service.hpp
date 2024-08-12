#pragma once

#include "settings.hpp"

#include "../containers/vector.hpp"
#include "service.hpp"

#include <cassert>

namespace aln
{
class SettingsRegistryService : public IService
{
  private:
    Vector<ISettings*> m_settings;

  public:
    // TODO: Hide from clients
    void RegisterSettings(ISettings* pSettings);
    void UnregisterSettings(ISettings* pSettings);

    template <typename T>
    T* GetSettings() const
    {
        static_assert(std::is_base_of_v<ISettings, T>);
        for (auto pCandidateSettings : m_settings)
        {
            auto pSettings = dynamic_cast<T*>(pCandidateSettings);
            if (pSettings != nullptr)
            {
                return pSettings;
            }
        }
        return nullptr;
    }
};
} // namespace aln