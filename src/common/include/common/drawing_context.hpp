#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <vector>

#include <common/colors.hpp>
#include <common/transform.hpp>
#include <common/vertex.hpp>

namespace aln
{
/// @todo Support multiple threads drawing at the same time
class DrawingContext
{
    friend class GraphicsSystem;

  private:
    std::vector<DebugVertex> m_vertices; // CPU debug lines data

  public:
    void DrawLine(const glm::vec3& start, const glm::vec3& end, const RGBColor color)
    {
        auto& startVertex = m_vertices.emplace_back();
        startVertex.pos = start;
        startVertex.color = color.Vec3();

        auto& endVertex = m_vertices.emplace_back();
        endVertex.pos = end;
        endVertex.color = color.Vec3();
    }

    /// @brief Draw the 3 coordinate axis of the given transform
    void DrawCoordinateAxis(const Transform& transform)
    {
        auto x = glm::rotate(transform.GetRotation(), glm::vec3(1, 0, 0));
        auto y = glm::rotate(transform.GetRotation(), glm::vec3(0, 1, 0));
        auto z = glm::rotate(transform.GetRotation(), glm::vec3(0, 0, 1));

        DrawLine(transform.GetTranslation(), transform.GetTranslation() + x, RGBColor::Red);   // x
        DrawLine(transform.GetTranslation(), transform.GetTranslation() + y, RGBColor::Green); // y
        DrawLine(transform.GetTranslation(), transform.GetTranslation() + z, RGBColor::Blue);  // z
    }
};
} // namespace aln