#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 camera_position;
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;

layout(location = 0) out vec3 o_color;
layout(location = 1) out vec2 o_tex_coord;
layout(location = 2) out vec3 o_normal;
layout(location = 3) out vec3 o_pos;

void main() {
    o_color = color;
    o_tex_coord = tex_coord;
    o_normal = mat3(transpose(inverse(ubo.model))) * normal;

    o_pos = vec3(ubo.model * vec4(position, 1.0));
    
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(position, 1.0);
}