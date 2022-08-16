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
    // Todo: pull out device
    std::shared_ptr<aln::vkg::Device> graphicsDevice;
    std::string defaultTexturePath;
    std::string defaultModelPath;
    std::string defaultSkeletonPath;
    std::string defaultMaterialPath;
};

} // namespace entities
} // namespace aln