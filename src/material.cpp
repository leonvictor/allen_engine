#include <glm/glm.hpp>

struct Material {
    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 diffuse;
    alignas(16) glm::vec3 specular;
    alignas(4) glm::vec1 shininess;

    Material() {
        ambient = glm::vec3(1.0f, 0.5f, 0.31f);
        diffuse = glm::vec3(1.0f, 0.5f, 0.31);
        specular = glm::vec3(0.5f, 0.5f, 0.5);
        shininess = glm::vec1(8.0f);
    }
};