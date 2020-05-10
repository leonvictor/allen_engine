#include <glm/glm.hpp>

namespace ecs::components {
    struct Transform {
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
    };
}