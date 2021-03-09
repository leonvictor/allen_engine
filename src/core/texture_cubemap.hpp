#pragma once

#include <vulkan/vulkan.hpp>

#include "context.hpp"
#include "device.hpp"
#include "image.hpp"
#include "texture.hpp"

namespace core
{

class TextureCubeMap : public core::Texture
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

    void loadFromDirectory(std::shared_ptr<core::Device> pDevice, std::string path);
};
} // namespace core