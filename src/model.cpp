#include<vector>
#include<unordered_map>

#include "vertex.hpp"
#include <tiny_obj_loader.h>


class Model {
    public:
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        glm::mat4 getModelMatrix() {
            return modelMatrix;
        }

        static Model fromObj(std::string path) {

            Model model = {};

            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;

            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
                throw std::runtime_error(warn + err);
            }

            std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

            for (const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                    Vertex vertex = {};
                    
                    vertex.pos = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                    };
                    
                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Flip the vertical component (.obj files assume 0 means bottom of immage, we assume it means top)
                    };

                    vertex.color = {1.0f, 1.0f, 1.0f};
                    
                    if (uniqueVertices.count(vertex) == 0) {
                        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                        model.vertices.push_back(vertex);
                    }
                    
                    model.indices.push_back(uniqueVertices[vertex]);
                }

                return model;
            }

        }

    private:
        glm::mat4 modelMatrix;
};