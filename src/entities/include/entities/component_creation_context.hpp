#pragma once

#include <memory>
#include <string>

namespace aln
{
namespace vkg
{
class Device;
}

namespace entities
{

struct ComponentCreationContext
{
    std::shared_ptr<aln::vkg::Device> graphicsDevice;
    std::string defaultTexturePath;
    std::string defaultModelPath;
};

} // namespace entities
} // namespace aln