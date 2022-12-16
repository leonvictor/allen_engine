#pragma once

#include <filesystem>

namespace aln::assets::converter
{

/// @todo Redo the image importer
namespace fs = std::filesystem;

struct ConverterConfig
{
    const fs::path inputDirectory;      // Input directory
    const fs::path rootOutputDirectory; // Output directory

    ConverterConfig(fs::path input, fs::path output) : inputDirectory(input), rootOutputDirectory(output) {}
    ConverterConfig(fs::path input) : inputDirectory(input), rootOutputDirectory(input.parent_path() / "assets_export") {}

    /// @brief Return the relative path from the current output directory to a given path
    fs::path RelativeToOutput(const fs::path& path) const;
};

/// @brief Convert an image into a texture asset.
/// @param input: Input path of the image.
/// @param output: Output path of the generated texture.
bool ConvertImage(const fs::path& input, const fs::path& output);
} // namespace aln::assets::converter