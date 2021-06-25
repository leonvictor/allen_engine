#pragma once

/// @brief Material ressource
/// TODO
struct Material
{
    std::string m_texturePath;

    // TODO: Load texture images (but not their vulkan objects)
    bool Load()
    {
        return true;
    }
    void Unload(){};
};