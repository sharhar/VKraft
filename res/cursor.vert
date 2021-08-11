#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec2 pos;

layout( location = 0 ) out struct vert_out {
    vec2 uv;
} OUT;

layout( push_constant ) uniform constants {
	vec2 screenSize;
} PC;

void main() {
	vec2 raw_pos = pos / PC.screenSize;
    gl_Position = vec4(2.0 * raw_pos.x, 2.0 * raw_pos.y, 0.0, 1.0);
	OUT.uv = raw_pos + 0.5;
}