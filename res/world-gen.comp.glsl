#version 430
layout(local_size_x = 16, local_size_y = 4, local_size_z = 16) in;

layout(binding = 0, std430) buffer a {
	uint data[];
} outArray;

layout( push_constant ) uniform constants {
	vec3 pos;
	uint seed;
} PC;

void main() {
	uint cube = 1;//PC.seed & 0xff;
	
	uint maskx = (gl_GlobalInvocationID.x & 1);
	uint maskz = (gl_GlobalInvocationID.z & 1);
	
	uint mask = (maskx | maskz);
	
	outArray.data[64 * gl_GlobalInvocationID.x + 4 * gl_GlobalInvocationID.z + gl_GlobalInvocationID.y] = ((cube << 24) | (cube << 16) | (cube << 8) | cube);
}
