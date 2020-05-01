#include <glm/glm.hpp>

struct Material {
    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;
    alignas(16) float shininess;

    Material() {
        ambient = {0.2f, 0.2f, 0.2f};
        diffuse = {0.5f, 0.5f, 0.5f};
        specular = {1.0f, 1.0f, 1.0f};
        shininess = 8;
    }
};