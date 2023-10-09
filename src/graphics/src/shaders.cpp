#include "shaders.hpp"

#include "device.hpp"

#include <shaderc/shaderc.hpp>

#include <fstream>

namespace aln::vkg::shaders
{
static bool ReadShaderFile(const std::filesystem::path& shaderFilePath, Vector<char>& out)
{
    // We start to read at the end of the file so we can use the read position to determine the size of the file to allocate a buffer
    auto file = std::ifstream(shaderFilePath, std::ios::ate | std::ios::binary);

    if (!file.is_open())
    {
        return false;
    }

    size_t fileSize = (size_t) file.tellg();
    out.resize(fileSize);

    // Go back to the start and read
    file.seekg(0);
    file.read(out.data(), fileSize);
    file.close();

    return true;
}

/// @brief Compiles a shader to a SPIR-V binary.
/// @param shaderSource source file buffer
/// @param shaderFileName source file name
/// @param output compiled shader buffer
/// @param kind shaderc kind of shader (vertex, frag, compute)
/// @param optimize whether to optimize the shader
/// @return the binary as a vector of 32-bit words.
static bool CompileGlslToSpvBinary(const Vector<char>& shaderSource, const char* shaderFileName, Vector<uint32_t>& outCompiledShaderSource, shaderc_shader_kind kind, bool optimize)
{
    assert(outCompiledShaderSource.empty());
    assert(!shaderSource.empty());

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

    if (optimize)
    {
        options.SetOptimizationLevel(shaderc_optimization_level_size);
    }

    auto compilationResult = compiler.CompileGlslToSpv(shaderSource.data(), (size_t) shaderSource.size(), kind, shaderFileName, options);
    if (compilationResult.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        assert(false); // TODO: Handle failures
        return false;
    }

    outCompiledShaderSource.assign(compilationResult.begin(), compilationResult.end());
    return true;
}

static vk::ShaderModule CreateShaderModule(Device* device, const std::filesystem::path& shaderFilePath)
{
    Vector<char> shaderData;
    ReadShaderFile(shaderFilePath, shaderData);

    vk::ShaderModuleCreateInfo shaderModuleCreateInfo;
    auto ext = shaderFilePath.extension();
    if (ext == ".spv")
    {
        shaderModuleCreateInfo.codeSize = shaderData.size() * sizeof(char);
        shaderModuleCreateInfo.pCode = reinterpret_cast<uint32_t*>(shaderData.data());

        return device->GetVkDevice().createShaderModule(shaderModuleCreateInfo).value;
    }
    else if (ext == ".vert" || ext == ".frag")
    {
        // TODO: Maybe infer entrypoint ?
        Vector<uint32_t> compiledShaderSource;
        CompileGlslToSpvBinary(shaderData, shaderFilePath.string().c_str(), compiledShaderSource, shaderc_glsl_infer_from_source, false);

        shaderModuleCreateInfo.codeSize = compiledShaderSource.size() * sizeof(uint32_t);
        shaderModuleCreateInfo.pCode =  compiledShaderSource.data();

        return device->GetVkDevice().createShaderModule(shaderModuleCreateInfo).value;
    }
    else
    {
        assert(false);
        return vk::ShaderModule();
    }
}

/// @brief Load a shader from a file.
/// @param shaderFilePath: shader file path (glsl or spirv)
/// @return the vulkan createInfo struct to add to a pipeline.
ShaderInfo LoadShader(Device* device, const std::filesystem::path& shaderFilePath, const vk::ShaderStageFlagBits stage, std::string entryPoint)
{
    ShaderInfo info = {
        .entryPoint = entryPoint,
        .module = CreateShaderModule(device, shaderFilePath),
        .stage = stage,
    };

    // pSpecializationInfo : We can set values for constants in the shader.
    // Then we can use a single shader module and have its behavior configured at pipeline creation (here)
    return info;
}


} // namespace aln::vkg::shaders