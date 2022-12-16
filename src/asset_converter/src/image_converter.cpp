#include "image_converter.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <assets/asset_archive_header.hpp>
#include <common/serialization/binary_archive.hpp>

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

    size_t size = width * height * desiredChannels;
    // Only RGBA8 format supported for now
    assert(desiredChannels == 4);

    std::vector<std::byte> data;
    BinaryMemoryArchive dataStream(data, IBinaryArchive::IOMode::Write);

    dataStream << width;
    dataStream << height;
    dataStream << size;
    dataStream.Write(pixels, size);

    // TODO: Compress again
    AssetArchiveHeader header("text"); // TODO: Use Texture::GetStaticAssetType();

    auto archive = BinaryFileArchive(output, IBinaryArchive::IOMode::Write);
    archive << header << data;

    stbi_image_free(pixels);
    return true;
}
} // namespace aln::assets::converter