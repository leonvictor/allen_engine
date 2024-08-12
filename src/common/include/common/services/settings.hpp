#pragma once

#include <cstdint>

namespace aln
{
class ISettings
{
  public:
    virtual uint32_t GetSettingsID() const = 0;
};
} // namespace aln