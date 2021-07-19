#include "mesh.hpp"

#include <tiny_obj_loader.h>
#include <utils/files.hpp>

namespace aln
{

void Mesh::Unload()
{
    m_vertices.clear();
    m_indices.clear();
    m_primitives.clear();
}

bool Mesh::Load()
{
    // TODO: Store supported file types as enum
    std::string fileExtension = utils::getFileExtension(m_sourceFile);
    if (fileExtension.compare("") == 0)
    {
        // throw std::runtime_error("No file extension found in: " + m_sourceFile);
        return false;
    }
    else if (fileExtension.compare(".obj"))
    {

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, m_sourceFile.c_str()))
        {
            return false;
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices = {};

        for (const auto& shape : shapes)
        {
            for (const auto& index : shape.mesh.indices)
            {
                // TODO: Build primitives on the go
                Vertex vertex;

                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

                if (index.texcoord_index >= 0)
                {
                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        1.0f - attrib.texcoords[2 * index.texcoord_index + 1] // Flip the vertical component (.obj files assume 0 means bottom of image, we assume it means top)
                    };
                }

                // vertex.color = glm::vec3(1.0f);

                if (index.normal_index >= 0)
                {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2],
                    };
                }

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
                    m_vertices.push_back(vertex);
                }

                m_indices.push_back(uniqueVertices[vertex]);
            }

            // TODO: Each shape is a primitive
        }

        m_primitives.push_back({0, (uint32_t) m_indices.size(), 0});
        return true;
    }
    return false;
}

void Mesh::RevertNormals()
{
    for (Vertex v : m_vertices)
    {
        v.normal = -v.normal;
    }
}
} // namespace aln