#pragma once

#include <string>
#include <vector>

#include "vertex.hpp"

// TODO: Complete gltf format
// In glTF, meshes are defined as arrays of primitives. Primitives correspond to the data required for GPU draw calls. Primitives specify one or more attributes, corresponding to the vertex attributes used in the draw calls.
struct PrimitiveComponent
{
    uint32_t firstIndex;
    uint32_t indexCount;
    uint32_t materialIndex;
};

/// @brief Mesh ressource
struct Mesh
{
    std::string m_sourceFile;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

    std::vector<PrimitiveComponent> m_primitives;

    void Unload()
    {
        m_vertices.clear();
        m_indices.clear();
        m_primitives.clear();
    }

    bool Load()
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
                        this->m_vertices.push_back(vertex);
                    }

                    this->m_indices.push_back(uniqueVertices[vertex]);
                }

                // TODO: Each shape is a primitive
            }

            this->m_primitives.push_back({0, (uint32_t) this->m_indices.size(), 0});
            return true;
        }
        return false;
    }

    void RevertNormals()
    {
        for (Vertex v : m_vertices)
        {
            v.normal = -v.normal;
        }
    }
};