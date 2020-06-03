#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_color;

layout(location = 0) out vec4 o_color;

void main() {
    o_color = vec4(in_color, 1.0f);
}