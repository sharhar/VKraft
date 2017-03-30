#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 pos;

layout( std140, binding = 0 ) uniform UniformBufferObject {
	mat4 proj;
	vec2 off;
} UBO;

layout( location = 0 ) out struct vert_out {
    vec2 uv;
} OUT;

void main() {
	vec4 screenCoord = UBO.proj * vec4(pos.x + UBO.off.x, pos.y + UBO.off.y, 0.0, 1.0);
    gl_Position = screenCoord;
	OUT.uv = vec2(screenCoord.x/2 + 0.5, screenCoord.y/2 + 0.5);
}