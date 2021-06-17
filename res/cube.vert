#version 450
#extension GL_ARB_separate_shader_objects : enable

#define PROP_FACE_MASK 7
#define PROP_FACE_EXP 0

#define PROP_POS_MASK 0xfff
#define PROP_POS_EXP 6

#define PROP_ID_MASK 0xfff
#define PROP_ID_EXP 18

layout ( location = 0 ) in int vert;
layout ( location = 1 ) in int state;

layout( location = 0 ) out struct vert_out {
    vec2 uv;
	float brightness;
} OUT;

int getProp(int num, int mask, int exp) {
	return (num >> exp) & mask;
}

int setProp(int num, int val, int mask, int exp) {
	return (val << exp) | (num & (~(mask << exp)));
}

int getTexID(int blockID) {
	return blockID - 1; // TODO: replace with texture id buffer lookup
}

layout( push_constant ) uniform constants {
	mat4 view;
	mat4 proj;
	vec3 chunkPos;
} PC;

struct FaceInfo {
	vec3 pos;
	ivec3 off0;
	ivec3 off1;
	float brightness;
};

FaceInfo faceInfos[6] = FaceInfo[6](
									FaceInfo(vec3(-0.5, -0.5,  0.5), ivec3(0, 1,  0), ivec3( 1,  0,  0), 0.86), // front face
									FaceInfo(vec3( 0.5, -0.5, -0.5), ivec3(0, 1,  0), ivec3(-1,  0,  0), 0.86), // back face
									FaceInfo(vec3(-0.5, -0.5, -0.5), ivec3(0, 1,  0), ivec3( 0,  0,  1), 0.80), // left face
									FaceInfo(vec3( 0.5, -0.5,  0.5), ivec3(0, 1,  0), ivec3( 0,  0, -1), 0.80), // right face
									FaceInfo(vec3(-0.5,  0.5,  0.5), ivec3(0, 0, -1), ivec3( 1,  0,  0), 0.70), // bottom face
									FaceInfo(vec3(-0.5, -0.5, -0.5), ivec3(0, 0,  1), ivec3( 1,  0,  0), 1.00) // top face
									);

void main() {
	FaceInfo face = faceInfos[getProp(state, PROP_FACE_MASK, PROP_FACE_EXP)];
	
	int v0 = vert & 1;
	int v1 = (vert >> 1) & 1;
	
	int chunk_pos_int = getProp(state, PROP_POS_MASK, PROP_POS_EXP);
	ivec3 chunk_pos_vec = ivec3(chunk_pos_int >> 8, -((chunk_pos_int >> 4) % 16), chunk_pos_int % 16);
	
	vec3 world_pos = vec3((v0 * face.off0) + (v1 * face.off1) + chunk_pos_vec) + face.pos + PC.chunkPos;
	
	gl_Position = PC.proj * PC.view * vec4(world_pos, 1.0);
	
	int cubeID = getTexID(getProp(state, PROP_ID_MASK, PROP_ID_EXP));
	
	OUT.uv = vec2(1 - v1 + (cubeID % 16), v0 + (cubeID / 16)) / 16.0;
	OUT.brightness = face.brightness;
}
