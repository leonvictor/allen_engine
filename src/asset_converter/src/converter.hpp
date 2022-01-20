#pragma once

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>

namespace aln::assets::converter
{

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

// Assimp methods
std::string GetAssimpMaterialName(const aiScene* pScene, int materialIndex);
std::string GetAssimpMeshName(const aiScene* pScene, int meshIndex);
void ExtractAssimpMaterials(const aiScene* pScene, const fs::path& input, const fs::path& outputFolder, const ConverterConfig& config);
void ExtractAssimpMeshes(const aiScene* pScene, const fs::path& input, const fs::path& output, const ConverterConfig& config);
void ExtractAssimpMatrix(aiMatrix4x4& in, std::array<float, 16>& out);
void ExtractAssimpNodes(const aiScene* pScene, const fs::path& input, const fs::path& outputFolder, const ConverterConfig& config);

/// @brief Convert an image into a texture asset.
/// @param input: Input path of the image.
/// @param output: Output path of the generated texture.
bool ConvertImage(const fs::path& input, const fs::path& output);
} // namespace aln::assets::converter