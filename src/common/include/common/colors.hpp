#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace aln
{

class RGBColor : public glm::vec3
{
  public:
    RGBColor() : glm::vec3(0.0f, 0.0f, 0.0f) {}
    RGBColor(float r, float g, float b) : glm::vec3(r, g, b) {}
};

class RGBAColor : public glm::vec4
{
  public:
    RGBAColor() : glm::vec4(0.0f, 0.0f, 0.0f, 1.0f) {}
    RGBAColor(float r, float g, float b, float a) : glm::vec4(r, g, b, a) {}
};
} // namespace aln