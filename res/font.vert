#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec2 vert_pos;
layout ( location = 1 ) in vec2 uv;

layout ( location = 2 ) in int charID;

layout( location = 0 ) out struct vert_out {
    vec2 uv;
} OUT;

layout( std140, binding = 0 ) uniform UniformBufferObject {
	mat4 proj;
	
	vec2 pos;
	float size;
} UBO;

void main() {
	int sxi = charID % 16;
	int syi = (charID - sxi) / 16;
	
	vec2 finalPos = UBO.pos + (vert_pos + vec2(gl_InstanceIndex, 0)) * UBO.size;
	
    gl_Position = UBO.proj * vec4(finalPos.xy, 0.0, 1.0);
	OUT.uv = vec2(float(sxi) + uv.x, float(syi) + uv.y) / 16.0;
}
