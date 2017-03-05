#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;

layout( location = 0 ) out struct vert_out {
    vec2 uv;
} OUT;

layout ( location = 1 ) out struct vert_out_sel {
	int selected;
} SEL;

layout( std140, binding = 0 ) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;

	vec3 selected;
} UBO;

void main() {
    gl_Position = UBO.proj * UBO.view * vec4(pos.x, 0.5 - pos.y, pos.z, 1.0);
	OUT.uv = uv;
	SEL.selected = 0;
}