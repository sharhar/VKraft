#version 430
layout(local_size_x = 1, local_size_y = 16, local_size_z = 16) in;

layout(binding = 0, std430) buffer a {
	int data[];
} outArray;

layout(binding = 1, std430) buffer b {
	int pos[];
} PC;

int perm[256] = int[256](151, 160, 137, 91, 90, 15, 131,
	13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23,
	190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32, 57, 177, 33,
	88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166,
	77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244,
	102, 143, 54, 65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196,
	135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123,
	5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42,
	223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
	129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228,
	251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107,
	49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254,
	138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
);

float grad(int hash, float x, float y) {
	int h = hash & 0x3F;  // Convert low 3 bits of hash code
	float h_less_than_4 = abs(sign(h&(~3)));  // 0 = yes, 1 = no
	float u = x + h_less_than_4*(y - x);  // into 8 simple gradient directions
	float v = y + h_less_than_4*(x - y);
	return ((1 - 2*(h & 1)) * u) + ((2 - 2*(h & 2)) * v); // and compute the dot product with (x,y)
}

float noise(float x, float y) {
	float n0, n1, n2;

	// Skewing/Unskewing factors for 2D
	float F2 = 0.366025403;  // F2 = (sqrt(3) - 1) / 2
	float G2 = 0.211324865;  // G2 = (3 - sqrt(3)) / 6   = F2 / (1 + 2 * K)

	// Skew the input space to determine which simplex cell we're in
	float s = (x + y) * F2;  // Hairy factor for 2D
	float xs = x + s;
	float ys = y + s;
	int i = int(floor(xs));
	int j = int(floor(ys));

	// Unskew the cell origin back to (x,y) space
	float t = (i + j) * G2;
	float X0 = i - t;
	float Y0 = j - t;
	float x0 = x - X0;  // The x,y distances from the cell origin
	float y0 = y - Y0;

	// For the 2D case, the simplex shape is an equilateral triangle.
	// Determine which simplex we are in.
	int i1, j1;  // Offsets for second (middle) corner of simplex in (i,j) coords
	if (x0 > y0) {   // lower triangle, XY order: (0,0)->(1,0)->(1,1)
		i1 = 1;
		j1 = 0;
	} else {   // upper triangle, YX order: (0,0)->(0,1)->(1,1)
		i1 = 0;
		j1 = 1;
	}

	// A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
	// a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
	// c = (3-sqrt(3))/6

	float x1 = x0 - i1 + G2;            // Offsets for middle corner in (x,y) unskewed coords
	float y1 = y0 - j1 + G2;
	float x2 = x0 - 1.0 + 2.0 * G2;   // Offsets for last corner in (x,y) unskewed coords
	float y2 = y0 - 1.0 + 2.0 * G2;

	// Work out the hashed gradient indices of the three simplex corners
	int gi0 = perm[i + perm[j]];
	int gi1 = perm[i + i1 + perm[j + j1]];
	int gi2 = perm[i + 1 + perm[j + 1]];

	// Calculate the contribution from the first corner
	float t0 = 0.5 - x0*x0 - y0*y0;
	if (t0 < 0.0) {
		n0 = 0.0;
	} else {
		t0 *= t0;
		n0 = t0 * t0 * grad(gi0, x0, y0);
	}

	// Calculate the contribution from the second corner
	float t1 = 0.5 - x1*x1 - y1*y1;
	if (t1 < 0.0) {
		n1 = 0.0;
	} else {
		t1 *= t1;
		n1 = t1 * t1 * grad(gi1, x1, y1);
	}

	// Calculate the contribution from the third corner
	float t2 = 0.5 - x2*x2 - y2*y2;
	if (t2 < 0.0) {
		n2 = 0.0;
	} else {
		t2 *= t2;
		n2 = t2 * t2 * grad(gi2, x2, y2);
	}

	// Add contributions from each corner to get the final noise value.
	// The result is scaled to return values in the interval [-1,1].
	return 45.23065 * (n0 + n1 + n2);
}

float fractal(float x, float y) {
	float result = 0.0;
	float denom  = 0.0;
	float frequency = 1.0 / 100.0;//mFrequency;
	float amplitude = 5.0;//mAmplitude;

	for (int i = 0; i < 8; i++) {
		result += (amplitude * noise(x * frequency, y * frequency));
		denom += amplitude;

		frequency *= 2.0;//mLacunarity;
		amplitude *= 0.5;//mPersistence;
	}

	return result;
}

void main() {
	uint chunkNum = gl_GlobalInvocationID.x;
	
	ivec2 worldPos = 16*ivec2(PC.pos[chunkNum*3 + 0], PC.pos[chunkNum*3 + 2]);
	
	outArray.data[256 * chunkNum + 16 * gl_GlobalInvocationID.z + gl_GlobalInvocationID.y] = 32 - int(fractal(int(gl_GlobalInvocationID.y) + worldPos.x, int(gl_GlobalInvocationID.z) + worldPos.y));
}
