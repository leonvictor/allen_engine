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

layout(binding = 2) uniform struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

// layout(binding = 2) uniform vec3 light_color; // TODO: Light color should be bound

void main() {
    
    vec3 light_color = vec3(1.0); // TODO: Pass this as uniform ?
    
    vec3 normal = normalize(in_normal);

    // Diffuse
    vec3 light_dir = normalize(ubo.light_position - in_position);
    float diff = clamp(dot(normal, light_dir), 0.0, 1.0);
    vec3 diffuse = light_color * (diff * material.diffuse);

    // Specular
    vec3 view_dir = normalize(ubo.camera_position - in_position);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular = material.specular * spec * light_color;

    // Ambient
    vec3 ambient = material.ambient * light_color; 

    // Combine lights
    vec3 result = (ambient + diffuse + specular) * in_color;
    o_color = vec4(result, 1.0); // TODO: Take light color into account
    // o_color = texture(texSampler, in_tex_coord); // Ignore texture for now
}