#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 pos;
layout ( location = 1 ) in vec4 rinf;

layout ( location = 0 ) out ivec4 out_rinf;

void main() {
    gl_Position = vec4(pos.x, 0.5 - pos.y, pos.z, 1.0);
	out_rinf = ivec4(int(rinf.x), int(rinf.y), int(rinf.z), int(rinf.w));
}