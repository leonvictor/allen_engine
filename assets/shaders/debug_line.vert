#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(vertex)

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 view_projection;
} ubo;

layout(location = 0) out vec3 o_color;

void main() {
    o_color = color;
    gl_Position = ubo.view_projection * vec4(position, 1.0);
}
