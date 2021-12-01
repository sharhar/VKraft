#version 430
layout(local_size_x = 16, local_size_y = 4, local_size_z = 16) in;

layout(binding = 0, std430) buffer a {
	uint data[];
} outArray;

layout(binding = 1, std430) buffer b {
	int[] pos;
} PC;

uint calcCubeAt(ivec3 pos) {
	return max(32 + int(10*sin(pos.x/10.0)*sin(pos.z/10.0)) - pos.y, 0);
}

void main() {
	uint chunkNum = gl_GlobalInvocationID.x >> 4;
	
	ivec3 worldPos = 16*ivec3(PC.pos[chunkNum*3 + 0], PC.pos[chunkNum*3 + 1], PC.pos[chunkNum*3 + 2]);
	
	ivec3 pos = ivec3((gl_GlobalInvocationID.x & 15) + worldPos.x, gl_GlobalInvocationID.z + worldPos.y, (gl_GlobalInvocationID.y * 4) + worldPos.z);
	
	outArray.data[64 * gl_GlobalInvocationID.x + 4 * gl_GlobalInvocationID.z + gl_GlobalInvocationID.y] = (
																										   (calcCubeAt(pos + ivec3(0, 0, 3)) << 24) |
																										   (calcCubeAt(pos + ivec3(0, 0, 2)) << 16) |
																										   (calcCubeAt(pos + ivec3(0, 0, 1)) << 8) |
																										    calcCubeAt(pos + ivec3(0, 0, 0)));
}
