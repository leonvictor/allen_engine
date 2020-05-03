#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 in_color;
layout(location = 1) in vec2 in_tex_coord;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec3 in_position;

layout(location = 0) out vec4 o_color;

layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 light_position;
    vec3 camera_position;
} ubo;

layout(set = 1, binding = 1) uniform sampler2D tex_sampler;

layout(set = 1, binding = 2) uniform Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
} material;

// Not used for now. TODO: Pass light info as uniform
struct Light {
    vec4 position; // position.w represents type of light
    vec4 direction; //direction.w represents range
    vec4 color; // color.w represents intensity
    // vec3 ambient;
    // vec3 diffuse;
    // vec3 specular;
};

layout(set = 0, binding = 0) buffer Lights {
    uint count;
    Light light[];
} lights;

// layout(binding = 2) uniform vec3 light_color; // TODO: Light color should be bound

vec3 apply_directional_light(uint index, vec3 normal) {
    // TODO: Pass as uniform
    vec3 light_ambient = vec3(0.2f, 0.2f, 0.2f);
    vec3 light_diffuse = vec3(0.5f);
    vec3 light_specular = vec3(1.0f);

    vec3 light_dir = normalize(-lights.light[index].direction.xyz);

    // Diffuse
    float diff = clamp(dot(normal, light_dir), 0.0, 1.0);
    vec3 diffuse = light_diffuse * (diff * vec3(texture(tex_sampler, in_tex_coord)));

    // Specular
    vec3 view_dir = normalize(ubo.camera_position - in_position);
    vec3 reflect_dir = reflect(-light_dir, normal);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    vec3 specular = light_specular * (material.specular * spec);

    return diffuse + specular;
}

vec3 apply_spot_light() {
    return vec3(0.0f);
}

vec3 apply_point_light() {
    return vec3(0.0f);
}

void main() {
    // TODO: Replace w/ specialization constants
    const int DIRECTIONNAL_LIGHT = 0;
    const int SPOT_LIGHT = 1;
    const int POINT_LIGHT = 2;

    // TODO Pass as uniform
    vec3 light_ambient = vec3(0.2f, 0.2f, 0.2f);
    vec3 light_diffuse = vec3(0.5f);
    vec3 light_specular = vec3(1.0f);
    
    vec3 normal = normalize(in_normal);
    vec3 result = vec3(0.0f);

    for (uint i = 0U; i < lights.count; i++) {
        if (lights.light[i].position.w == DIRECTIONNAL_LIGHT) {
            result += apply_directional_light(i, normal);
        }
        if (lights.light[i].position.w == SPOT_LIGHT) {

        }
        if (lights.light[i].position.w == POINT_LIGHT) {

        }
    }
    // Diffuse
    // vec3 light_dir = normalize(ubo.light_position - in_position);
    // float diff = clamp(dot(normal, light_dir), 0.0, 1.0);
    // // vec3 diffuse = light_diffuse * (diff * material.diffuse);
    // vec3 diffuse = light_diffuse * (diff * vec3(texture(tex_sampler, in_tex_coord)));

    // // Specular
    // vec3 view_dir = normalize(ubo.camera_position - in_position);
    // vec3 reflect_dir = reflect(-light_dir, normal);
    // float spec = pow(max(dot(view_dir, reflect_dir), 0.0), material.shininess);
    // vec3 specular = light_specular * (material.specular * spec);

    // Ambient
    // vec3 ambient = light_ambient * material.ambient; 
    vec3 ambient = light_ambient * vec3(texture(tex_sampler, in_tex_coord)); 

    // Combine lights
    // vec3 result = (ambient + diffuse + specular);
    result += ambient;
    o_color = vec4(result, 1.0);
}