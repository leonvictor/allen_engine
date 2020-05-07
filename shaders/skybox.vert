#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;

layout (binding = 0) uniform UBO 
{
	mat4 model;
    mat4 view;
    mat4 projection;
    vec3 lightpos;
    vec3 camera_position;
} ubo;

layout (location = 0) out vec3 outUVW;

void main() 
{
	outUVW = inPos;
	gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);
}
