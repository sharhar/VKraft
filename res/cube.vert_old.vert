#version 450
#extension GL_ARB_separate_shader_objects : enable

layout ( location = 0 ) in vec4 pos;
layout ( location = 1 ) in vec2 uv;

layout( location = 0 ) out struct vert_out {
    vec2 uv;
} OUT;

layout ( location = 1 ) out struct vert_out_sel {
	int selected;
} SEL;

layout( location = 2 ) out struct vert_out_vis {
    float visibility;
} VIS;

layout( std140, binding = 0 ) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;

	vec3 selected;

	float density;
	float gradient;
} UBO;

void main() {
	int offID = int(pos.w);
	
	vec3 off = vec3(0.0, 0.0, 0.0);

	if((offID & 4) != 0) {
		off.x = -0.5;
	} else {
		off.x = 0.5;
	}

	if((offID & 2) != 0) {
		off.y = -0.5;
	} else {
		off.y = 0.5;
	}

	if((offID & 1) != 0) {
		off.z = -0.5;
	} else {
		off.z = 0.5;
	}

	vec3 cpos = vec3(pos.xyz) + off;

	vec4 realtiveWorldPos = UBO.view * vec4(pos.x, 0.5 - pos.y, pos.z, 1.0);
    gl_Position = UBO.proj * realtiveWorldPos;
	OUT.uv = uv;

	if(cpos.x == UBO.selected.x && cpos.y == UBO.selected.y && cpos.z == UBO.selected.z) {
		SEL.selected = 1;
	} else {
		SEL.selected = 0;
	}

	float distance = length(vec3(realtiveWorldPos.xyz));
	VIS.visibility = exp(-pow((distance*UBO.density), UBO.gradient));
	VIS.visibility = clamp(VIS.visibility, 0.0, 1.0);
}