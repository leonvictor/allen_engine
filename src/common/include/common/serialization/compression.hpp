#pragma once

#include <common/containers/vector.hpp>

#include <lz4.h>

/// --------------------------
/// Compression
/// @note This version creates a copy of the data in order to compress/decompress it
/// A cleverer alternative probably exists
/// --------------------------

namespace aln::Compression
{
/// @brief Compress the archive
/// @return The original size of the uncompressed archive
static uint32_t Compress(Vector<std::byte>& data)
{
    auto uncompressedSize = static_cast<int>(data.size());
    auto maxCompressedSize = LZ4_compressBound(uncompressedSize);

    auto buffer = Vector<std::byte>(maxCompressedSize);

    auto compressedSize = LZ4_compress_default(
        reinterpret_cast<const char*>(data.data()),
        reinterpret_cast<char*>(buffer.data()),
        uncompressedSize,
        maxCompressedSize);

    buffer.resize(compressedSize);
    data.swap(buffer);

    return uncompressedSize;
}

static void Decompress(Vector<std::byte>& data, uint32_t originalSize)
{
    auto buffer = Vector<std::byte>(originalSize);

    LZ4_decompress_safe(
        reinterpret_cast<const char*>(data.data()),
        reinterpret_cast<char*>(buffer.data()),
        static_cast<int>(data.size()),
        static_cast<int>(originalSize));

    data.swap(buffer);
}

} // namespace aln::Compression