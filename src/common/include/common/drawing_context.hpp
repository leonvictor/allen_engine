#pragma once


#include <common/colors.hpp>
#include <common/transform.hpp>
#include <common/vertex.hpp>
#include <common/maths/vec3.hpp>
#include <common/containers/vector.hpp>

namespace aln
{
/// @todo Support multiple threads drawing at the same time
class DrawingContext
{
    friend class WorldRenderingSystem;

  private:
    Vector<DebugVertex> m_vertices; // CPU debug lines data

  public:
    void DrawLine(const Vec3& start, const Vec3& end, const RGBColor color)
    {
        auto& startVertex = m_vertices.emplace_back();
        startVertex.pos = start;
        startVertex.color = static_cast<Vec3>(color);

        auto& endVertex = m_vertices.emplace_back();
        endVertex.pos = end;
        endVertex.color = static_cast<Vec3>(color);
    }

    /// @brief Draw the 3 coordinate axis of the given transform
    void DrawCoordinateAxis(const Transform& transform)
    {
        auto x = transform.GetAxisX();
        auto y = transform.GetAxisY();
        auto z = transform.GetAxisZ();

        DrawLine(transform.GetTranslation(), transform.GetTranslation() + x, RGBColor::Red);   // x
        DrawLine(transform.GetTranslation(), transform.GetTranslation() + y, RGBColor::Green); // y
        DrawLine(transform.GetTranslation(), transform.GetTranslation() + z, RGBColor::Blue);  // z
    }
};
} // namespace aln