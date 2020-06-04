#pragma once

#include <glm/glm.hpp>

struct ColorUID {
    static int current;
    int id;

    ColorUID() {
        id = current;
        current++;
    }

    ColorUID(glm::vec3 rgb)
    {
        id = rgb.r + rgb.g * 256 + rgb.b * 256*256;
    }

    glm::vec3 toRGB() {
        int r = (id & 0x000000FF) >>  0;
        int g = (id & 0x0000FF00) >>  8;
        int b = (id & 0x00FF0000) >> 16;

        return glm::vec3(r, g, b);
    }

    bool operator==(ColorUID other) {
        return id == other.id;
    }

    bool operator!=(ColorUID other)
    {
        return id != other.id;
    }
};

int ColorUID::current;