#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;

layout( std140, binding = 0 ) uniform UniformBufferObject {
	mat4 proj;
} UBO;

layout( location = 0 ) out struct vert_out {
    vec2 uv;
} OUT;

void main() {
    gl_Position = UBO.proj * vec4(pos.xy, 0.0, 1.0);
	OUT.uv = uv;
}