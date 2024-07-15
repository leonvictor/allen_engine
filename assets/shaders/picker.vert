#version 450
#extension GL_ARB_separate_shader_objects : enable
#pragma shader_stage(vertex)

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 camera_position;
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
// TODO: tex_coord and normal are not needed
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;

layout(location = 0) out vec3 o_color;

void main() {
    o_color = color;
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(position, 1.0);
}
