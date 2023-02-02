#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_debug_printf : enable
#pragma shader_stage(vertex)

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec3 in_color;
layout (location = 2) in vec2 in_texcoord;
layout (location = 3) in vec3 in_normal;
layout (location = 4) in vec4 in_weights;
layout (location = 5) in uvec4 in_bone_indices;

layout (set = 0, binding = 0) uniform SceneUBO
{
	mat4 view;
	mat4 projection;
	vec3 camera_position;
} sceneData;

layout(std430, set = 3, binding = 0) readonly buffer ModelMatrices {
	mat4 modelMatrices[];
};

layout(std430, set = 4, binding = 0) readonly buffer JointMatrices {
	mat4 jointMatrices[];
};

layout (push_constant) uniform constants
{
	uint bones_count;
	uint bones_start_index;
} pushConstants;

layout (location = 0) out vec3 out_color;
layout (location = 1) out vec2 out_tex_coord;
layout (location = 2) out vec3 out_normal;
layout (location = 3) out vec3 out_pos;

void main() 
{
	out_color = in_color;
	out_tex_coord = in_texcoord;
	out_normal = in_normal;
    out_pos = vec3(modelMatrices[gl_InstanceIndex] * vec4(in_pos, 1.0));
	// debugPrintfEXT("out_pos (%f, %f, %f)", out_pos.x, out_pos.y, out_pos.z);

	// Calculate skinned matrix from weights and joint indices of the current vertex
	mat4 skinMat = 
		in_weights.x * jointMatrices[pushConstants.bones_start_index + int(in_bone_indices.x)] +
		in_weights.y * jointMatrices[pushConstants.bones_start_index + int(in_bone_indices.y)] +
		in_weights.z * jointMatrices[pushConstants.bones_start_index + int(in_bone_indices.z)] +
		in_weights.w * jointMatrices[pushConstants.bones_start_index + int(in_bone_indices.w)];

	gl_Position = sceneData.projection * sceneData.view * modelMatrices[gl_InstanceIndex] * skinMat * vec4(in_pos.xyz, 1.0);
	
	out_normal = normalize(transpose(inverse(mat3(sceneData.view * modelMatrices[gl_InstanceIndex] * skinMat))) * in_normal);

	// vec4 pos = uboScene.view * vec4(in_pos, 1.0);
	// vec3 lPos = mat3(uboScene.view) * uboScene.lightPos.xyz;
	// outLightVec = lPos - pos.xyz;
	// outViewVec = -pos.xyz;
}