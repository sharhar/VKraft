#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PROP_FACE_MASK 63
#define PROP_FACE_EXP 0

#define PROP_POS_MASK 0xfff
#define PROP_POS_EXP 6

#define PROP_ID_MASK 0xfff
#define PROP_ID_EXP 18

layout ( location = 0 ) in int vert;
layout ( location = 1 ) in int state;

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

int getProp(int num, int mask, int exp) {
	return (num >> exp) & mask;
}

int setProp(int num, int val, int mask, int exp) {
	return (val << exp) | (num & (~(mask << exp)));
}

int getTexID(int blockID) {
	return blockID - 1; // TODO: replace with texture id buffer lookup
}

void main() {
	gl_Position = vec4(-500.0, -500.0, -500.0, 1.0);
	OUT.uv = vec2(0.0);
	
	int blockID = getProp(state, PROP_ID_MASK, PROP_ID_EXP);
	int vid = getProp(state, PROP_FACE_MASK, PROP_FACE_EXP);
	int face = (vert >> 5) & 63;
	
	if (blockID != 0 && (face & vid) != 0) {
		int posi = getProp(state, PROP_POS_MASK, PROP_POS_EXP);
		int posx = posi >> 8;
		int posy = (posi >> 4) % 16;
		int posz = posi % 16;
		
		int cubeID = getTexID(blockID);
		
		vec3 pos = vec3(-0.5, -0.5, -0.5);
		
		pos.x = pos.x + float(vert & 1);
		pos.y = pos.y + float((vert >> 1) & 1);
		pos.z = pos.z + float((vert >> 2) & 1);
		
		vec2 uv = vec2(float((vert >> 3) & 1), float((vert >> 4) & 1));
		
		gl_Position = UBO.proj * UBO.view * vec4(pos.x + float(posx), pos.y + float(posy), pos.z + float(posz), 1.0);
		OUT.uv = (uv + vec2(cubeID % 16, cubeID / 16)) / 16.0;
	}
}
