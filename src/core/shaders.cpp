#include "shaders.hpp"
#include "../utils/files.cpp"
#include "device.hpp"
#include <iostream>
#include <vulkan/vulkan.hpp>

namespace core::shaders
{

vk::ShaderModule createShaderModule(std::shared_ptr<core::Device> device, const std::string& filename)
{
    std::string ext = utils::getFileExtension(filename);

    if (ext == "spv")
    {
        std::vector<char> code = utils::readFile(filename);
        return device->logical->createShaderModule({{}, code.size() * sizeof(char), reinterpret_cast<uint32_t*>(code.data())});
    }
    else if (ext == "vert" || ext == "frag")
    {
        // TODO: Maybe infer entrypoint ?
        std::vector<uint32_t> code = compileGlslToSpvBinary(filename, shaderc_glsl_infer_from_source, false);
        return device->logical->createShaderModule({{}, code.size() * sizeof(uint32_t), code.data()});
    }
    else
    {
        throw;
    }
}

vk::PipelineShaderStageCreateInfo loadShader(std::shared_ptr<core::Device> device, const std::string& filename, const vk::ShaderStageFlagBits stage, const std::string& entryPoint)
{
    auto shaderModule = createShaderModule(device, filename);
    vk::PipelineShaderStageCreateInfo createInfo;
    createInfo.stage = stage;
    createInfo.module = shaderModule;
    createInfo.pName = strdup(entryPoint.c_str());
    // pSpecializationInfo : We can set values for constants in the shader.
    // Then we can use a single shader module and have its behavior configured at pipeline creation (here)
    return createInfo;
}

/// @brief Compiles a shader to a SPIR-V binary.
///
/// @param source_name glsl shader file name
/// @param kind shaderc kind of shader (vertex, frag, compute)
/// @param optimize whether to optimize the shader
///
/// @return the binary as a vector of 32-bit words.
std::vector<uint32_t> compileGlslToSpvBinary(const std::string& source_name,
                                             shaderc_shader_kind kind,
                                             bool optimize)
{
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

    std::vector<char> source = utils::readFile(source_name);

    if (optimize)
        options.SetOptimizationLevel(shaderc_optimization_level_size);

    shaderc::SpvCompilationResult module =
        compiler.CompileGlslToSpv(std::string(source.begin(), source.end()), kind, source_name.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cerr << module.GetErrorMessage();
        // TODO: Custom exception type (something like ShaderCompilationException)
        throw;
    }

    return {module.cbegin(), module.cend()};
}
} // namespace core::shaders