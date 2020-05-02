#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec3 in_position;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 light_position;
    vec3 camera_position;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

layout(binding = 2) uniform Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

// Not used for now. TODO: Pass light info as uniform
struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// layout(binding = 2) uniform vec3 light_color; // TODO: Light color should be bound

void main() {
    // TODO Pass as uniform
    vec3 light_ambient = vec3(0.2f, 0.2f, 0.2f);
    vec3 light_diffuse = vec3(0.5f);
    vec3 light_specular = vec3(1.0f);
    
    vec3 normal = normalize(in_normal);

    // Diffuse
    vec3 light_dir = normalize(ubo.light_position - in_position);
    float diff = clamp(dot(normal, light_dir), 0.0, 1.0);
    // vec3 diffuse = light_diffuse * (diff * material.diffuse);
    vec3 diffuse = light_diffuse * (diff * vec3(texture(texSampler, in_tex_coord)));

    // Specular
    vec3 view_dir = normalize(ubo.camera_position - in_position);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular = light_specular * (material.specular * spec);

    // Ambient
    // vec3 ambient = light_ambient * material.ambient; 
    vec3 ambient = light_ambient * vec3(texture(texSampler, in_tex_coord)); 

    // Combine lights
    vec3 result = (ambient + diffuse + specular) * in_color;
    o_color = vec4(result, 1.0);
}