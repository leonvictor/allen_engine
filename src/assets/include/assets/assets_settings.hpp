#pragma once

#include <common/serialization/hash.hpp>
#include <common/services/settings.hpp>

#include <filesystem>

namespace aln
{
class AssetsSettings : public ISettings
{
  public:
    std::filesystem::path m_workingDirectory;
    std::filesystem::path m_projectAssetsDirectory;
    std::filesystem::path m_editorAssetsDirectory;

    inline uint32_t GetSettingsID() const override { return Hash32("AssetsSettings"); }
};
} // namespace aln