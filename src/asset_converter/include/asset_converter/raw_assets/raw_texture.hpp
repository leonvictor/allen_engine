#pragma once

#include <assert.h>
#include <filesystem>

#include <assets/asset_archive_header.hpp>
#include <common/serialization/binary_archive.hpp>

#include "../assimp_scene_context.hpp"
#include "raw_asset.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace aln::assets::converter
{

class RawTexture : public IRawAsset
{
    friend class AssimpTextureReader;
    friend class FileTextureReader;

    int m_width;
    int m_height;
    int m_channels;

    unsigned char* m_pixelData;

    void Serialize(BinaryMemoryArchive& archive) final override
    {
        size_t imageSize = m_width * m_height * m_channels;

        archive << m_width;
        archive << m_height;
        archive << imageSize;
        archive.Write(m_pixelData, imageSize);
    }

    ~RawTexture()
    {
        stbi_image_free(m_pixelData);
    }
};

struct AssimpTextureReader
{
    static AssetID ReadTexture(const AssimpSceneContext& sceneContext, const aiTexture* pTexture, const std::filesystem::path& outPath)
    {
        RawTexture texture;
        texture.m_id = AssetID(outPath.string());
        texture.m_channels = 4;

        int size = pTexture->mHeight == 0 ? pTexture->mWidth * 4 : pTexture->mHeight * pTexture->mWidth;

        int actualChannels;
        texture.m_pixelData = stbi_load_from_memory((unsigned char*) pTexture->pcData, size, &texture.m_width, &texture.m_height, &actualChannels, 4);

        if (texture.m_pixelData == nullptr)
        {
            // TODO: Do not throw but rather log an error
            throw std::runtime_error("Failed to load embedded image");
            return AssetID();
        }

        // Save to disk
        // TODO: Compress again
        Vector<std::byte> data;
        BinaryMemoryArchive dataStream(data, IBinaryArchive::IOMode::Write);

        texture.Serialize(dataStream);

        AssetArchiveHeader header("text"); // TODO: Use Texture::GetStaticAssetType();

        auto archive = BinaryFileArchive(outPath, IBinaryArchive::IOMode::Write);
        archive << header << data;

        return texture.m_id;
    }
};

struct FileTextureReader
{
    static AssetID ReadTexture(const std::filesystem::path& imagePath, const std::filesystem::path& outPath)
    {
        assert(std::filesystem::exists(imagePath));

        RawTexture texture;
        texture.m_id = AssetID(outPath.string());
        texture.m_channels = 4; // RGBA

        int actualChannels;
        if (!stbi_info(imagePath.string().c_str(), &texture.m_width, &texture.m_height, &actualChannels))
        {
            std::cout << "Unsupported file format" << std::endl;
            return AssetID();
        }

        texture.m_pixelData = stbi_load(imagePath.string().c_str(), &texture.m_width, &texture.m_height, &actualChannels, texture.m_channels);

        if (texture.m_pixelData == nullptr)
        {
            // TODO: Do not throw but rather log an error
            throw std::runtime_error("Failed to load image at " + imagePath.string());
            return AssetID();
        }

        // Save to disk
        // TODO: Compress again
        Vector<std::byte> data;
        BinaryMemoryArchive dataStream(data, IBinaryArchive::IOMode::Write);

        texture.Serialize(dataStream);

        AssetArchiveHeader header("text"); // TODO: Use Texture::GetStaticAssetType();

        auto archive = BinaryFileArchive(outPath, IBinaryArchive::IOMode::Write);
        archive << header << data;

        return texture.m_id;
    }
};
} // namespace aln::assets::converter