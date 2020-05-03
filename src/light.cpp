#include <glm/glm.hpp>

enum LightType {
    Directionnal,
    Spot,
    Point
};

struct LightUniform {
    alignas(16) glm::vec4 position; // position.w represents type of light
    alignas(16) glm::vec3 direction; //direction.w represents range
    alignas(16) glm::vec3 color; // color.w represents intensity
};

class Light {
public:
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;

    float intensity = 1.0f;
    float range = 1.0f;

    LightType type;

    LightUniform getUniform() {
        LightUniform u;
        u.position = glm::vec4(position, (float) type);
        u.direction = glm::vec4(direction, range);
        u.color = glm::vec4(color, intensity);

        return u;
    }
};