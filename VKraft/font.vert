#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec2 pos;
layout ( location = 1 ) in vec2 uv;

layout( location = 0 ) out struct vert_out {
    vec2 uv;
} OUT;

layout( std140, binding = 0 ) uniform UniformBufferObject {
	mat4 proj;
} UBO;

void main() {
    gl_Position = UBO.proj * vec4(pos.xy, 0.0, 1.0);
	OUT.uv = uv;
}