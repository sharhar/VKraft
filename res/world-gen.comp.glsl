#version 430
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, std430) buffer a {
	float data[];
} outArray;

layout( push_constant ) uniform constants {
	vec3 pos;
	int seed;
} PC;

void main() {
	vec3 mask = vec3((PC.seed & 4) >> 2, (PC.seed & 2) >> 1, PC.seed & 1);
	
	outArray.data[gl_GlobalInvocationID.x] = dot(PC.pos, mask);
}
