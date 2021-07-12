#pragma once

#include "texture.hpp"
#include <vulkan/vulkan.hpp>

namespace vkg
{

// fwd
class Device;

class TextureCubeMap : public Texture
{
  private:
  public:
    // TODO: Quick and dirty way of storing faces names for now
    // Generate optimized file ?
    std::array<std::string, 6> faces = {
        "Right",
        "Left",
        "Up",
        "Down",
        "Front",
        "Back",
    };

    void LoadFromDirectory(std::shared_ptr<Device> pDevice, std::string path);
};
} // namespace vkg