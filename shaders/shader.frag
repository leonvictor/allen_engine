#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 fragPos;
layout(location = 4) in vec3 lightDir;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texSampler;
// layout(binding = 2) uniform vec3 lightColor; // TODO: Light color should be bound

void main() {
    vec3 lightColor = vec3(1.0); // TODO: Pass this as uniform ?
    // outColor = texture(texSampler, fragTexCoord); // Ignore texture for now
    vec3 norm = normalize(normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor; 

    vec3 result = (ambient + diffuse) * fragColor;
    outColor = vec4(result, 1.0); // TODO: Take light color into account
}