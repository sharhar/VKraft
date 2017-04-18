#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 pos;
layout (location = 1) in vec2 uv;

layout( location = 0 ) out struct vert_out {
    vec2 uv;
} OUT;

void main() {
    gl_Position = vec4(pos.xy, 0.5, 1.0);
	OUT.uv = uv;
}