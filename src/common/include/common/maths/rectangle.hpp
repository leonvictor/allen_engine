#pragma once

namespace aln
{
struct Rectangle
{
    uint32_t width = 0;
    uint32_t height = 0;

    float GetArea() const { return width * height; }
    float GetPerimeter() const { return (width + height) * 2; }
};
} // namespace aln