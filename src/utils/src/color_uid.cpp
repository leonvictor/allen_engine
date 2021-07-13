#pragma once

#include <glm/glm.hpp>

namespace aln::utils
{
struct ColorUID
{
    static uint32_t current;
    uint32_t id;

    ColorUID() {}

    explicit ColorUID(const glm::vec3& rgb)
    {
        id = rgb.r + rgb.g * 256 + rgb.b * 256 * 256;
    }

    ColorUID(const unsigned int& r, const unsigned int& g, const unsigned int& b)
    {
        id = r + g * 256 + b * 256 * 256;
    }

    void generate()
    {
        current++;
        id = current;
    }

    glm::vec3 const toRGB()
    {
        int r = (id & 0x000000FF) >> 0;
        int g = (id & 0x0000FF00) >> 8;
        int b = (id & 0x00FF0000) >> 16;

        // TODO: Uniformize usage of floats (because we pass float values anyway)
        return glm::vec3((float) r / 256, (float) g / 256, (float) b / 256);
    }

    bool operator==(ColorUID other)
    {
        return id == other.id;
    }

    bool operator!=(ColorUID other)
    {
        return id != other.id;
    }
};

inline bool operator<(const ColorUID& a, const ColorUID& b)
{
    return a.id < b.id;
}

uint32_t ColorUID::current;
}