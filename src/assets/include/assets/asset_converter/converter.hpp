#pragma once
#include <filesystem>

namespace fs = std::filesystem;

/// @brief Convert an image into a texture asset.
/// @param input: Input path of the image.
/// @param output: Output path of the generated texture.
bool ConvertImage(const fs::path& input, const fs::path& output);