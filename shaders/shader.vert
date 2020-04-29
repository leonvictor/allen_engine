#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 lightpos;
} ubo;

layout(location = 0) in vec3 inPositions;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec3 fragPos;
layout(location = 4) out vec3 lightDir;

void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPositions, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    normal = inNormal;
    fragPos = vec3(ubo.model * vec4(inPositions, 1.0));
    lightDir = normalize(ubo.lightpos - fragPos);
}