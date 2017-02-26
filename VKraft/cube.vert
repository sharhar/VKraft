#version 400
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 pos;
layout (location = 1) in vec4 rinf;

layout( location = 0 ) out struct vert_out {
    ivec4 rinf;
} OUT;

void main() {
    gl_Position = vec4(pos.xyz, 1.0);
	OUT.rinf = ivec4(int(rinf.x), int(rinf.y), int(rinf.z), int(rinf.w));
}