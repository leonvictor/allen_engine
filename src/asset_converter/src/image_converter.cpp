#include "image_converter.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <assets/asset_system/texture_asset.hpp>

#include <iostream>

namespace aln::assets::converter
{
namespace fs = std::filesystem;

bool ConvertImage(const fs::path& input, const fs::path& output)
{
    int width, height, channels, ok;
    ok = stbi_info(input.string().c_str(), &width, &height, &channels);
    if (!ok)
    {
        std::cout << "Unsupported file format" << std::endl;
        return false;
    }

    int desiredChannels = STBI_rgb_alpha;
    stbi_uc* pixels = stbi_load(input.string().c_str(), &width, &height, &channels, desiredChannels);

    if (!pixels)
    {
        // TODO: Do not throw but rather log an error
        throw std::runtime_error("Failed to load image at " + input.string());
        return false;
    }

    uint32_t size = width * height * desiredChannels;
    TextureFormat format;
    if (desiredChannels == 3)
    {
        format = TextureFormat::RGB8;
    }
    else if (desiredChannels == 4)
    {
        format = TextureFormat::RGBA8;
    }
    else
    {
        stbi_image_free(pixels);
        std::cout << "Unsupported file format" << std::endl;
        return false;
    }

    TextureInfo info =
        {
            .size = size,
            .format = format,
            .pixelSize = {(uint32_t) width, (uint32_t) height, (uint32_t) desiredChannels},
            .originalFile = input.string(),
        };

    AssetFile image = PackTexture(&info, pixels);
    stbi_image_free(pixels);
    SaveBinaryFile(output.string(), image);
    return true;
}
} // namespace aln::assets::converter