#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec3 pos;
layout ( location = 1 ) in vec2 uv;

layout( location = 0 ) out struct vert_out {
    vec2 uv;
} OUT;

layout( std140, binding = 0 ) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;

	vec3 selected;

	float density;
	float gradient;
} UBO;

void main() {
	int cubeID = 3;
	
	int posx = gl_InstanceIndex / 256;
	int posy = (gl_InstanceIndex / 16) % 16;
	int posz = gl_InstanceIndex % 16;
	
	gl_Position = UBO.proj * UBO.view * vec4(pos.x + float(posx), pos.y + float(posy), pos.z + float(posz), 1.0);
	OUT.uv = (uv + vec2(cubeID % 16, cubeID / 16)) / 16.0;
}
