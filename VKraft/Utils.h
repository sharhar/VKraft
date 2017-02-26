#pragma once

#define DEG_TO_RAD 0.0174532925199

#include <fstream>
#include <string>
#include <math.h>

typedef struct Vec3 {
	float x, y, z;

	Vec3 add(Vec3 input) {
		return{ x + input.x, y + input.y , z + input.z };
	}

	float dist(Vec3 other) {
		float xd = x - other.x;
		float yd = y - other.y;
		float zd = z - other.z;
		return sqrt(xd*xd + yd*yd + zd*zd);
	}
} Vec3;

typedef struct Vec3i {
	int x, y, z;

	Vec3i add(Vec3i input) {
		return{ x + input.x, y + input.y , z + input.z };
	}
} Vec3i;

static std::string readFile_c(std::string path) {
	std::string result;

	std::string line;
	std::ifstream resultFile(path);
	if (resultFile.is_open()) {
		while (getline(resultFile, line)) {
			result += line + "\n";
		}

		resultFile.close();
	}

	return result;
}

static float* getPerspective() {
	float aspect = 16.0f / 9.0f;
	float fov = 90;
	float farr = 1000;
	float nearr = 0.01f;

	float t = tan(DEG_TO_RAD*fov / 2);

	float* projection = new float[16];
	projection[0] = 1 / (t*aspect);
	projection[1] = 0;
	projection[2] = 0;
	projection[3] = 0;

	projection[4] = 0;
	projection[5] = 1 / t;
	projection[6] = 0;
	projection[7] = 0;

	projection[8] = 0;
	projection[9] = 0;
	projection[10] = -(farr + nearr) / (farr - nearr);
	projection[11] = -1;

	projection[12] = 0;
	projection[13] = 0;
	projection[14] = -(2 * farr*nearr) / (farr - nearr);
	projection[15] = 0;

	return projection;
}

static void getWorldview(float* mat, Vec3 pos, Vec3 rot) {
	float cx = cos(rot.x*DEG_TO_RAD);
	float sx = sin(rot.x*DEG_TO_RAD);
	float cy = cos(rot.y*DEG_TO_RAD);
	float sy = sin(rot.y*DEG_TO_RAD);
	float cz = cos(rot.z*DEG_TO_RAD);
	float sz = sin(rot.z*DEG_TO_RAD);

	mat[0] = (cy*cz);
	mat[1] = (sx*sy*cz + cx*sz);
	mat[2] = (sx*sz - cx*sy*cz);
	mat[3] = 0;

	mat[4] = (-cy*sz);
	mat[5] = (cx*cz - sx*sy*sz);
	mat[6] = (cx*sy*sz + sx*cz);
	mat[7] = 0;

	mat[8] = (sy);
	mat[9] = (-sx*cy);
	mat[10] = (cx*cy);
	mat[11] = 0;

	mat[12] = -(mat[0] * pos.x + mat[4] * pos.y + mat[8] * pos.z);
	mat[13] = -(mat[1] * pos.x + mat[5] * pos.y + mat[9] * pos.z);
	mat[14] = -(mat[2] * pos.x + mat[6] * pos.y + mat[10] * pos.z);
	mat[15] = 1;
}


static void getModelview(float* mat, Vec3 pos, Vec3 rot, Vec3 scale) {
	float cx = cos(rot.x*DEG_TO_RAD);
	float sx = sin(rot.x*DEG_TO_RAD);
	float cy = cos(rot.y*DEG_TO_RAD);
	float sy = sin(rot.y*DEG_TO_RAD);
	float cz = cos(rot.z*DEG_TO_RAD);
	float sz = sin(rot.z*DEG_TO_RAD);

	mat[0] = (cy*cz)*scale.x;
	mat[1] = (sx*sy*cz + cx*sz)*scale.x;
	mat[2] = (sx*sz - cx*sy*cz)*scale.x;
	mat[3] = 0;

	mat[4] = (-cy*sz)*scale.y;
	mat[5] = (cx*cz - sx*sy*sz)*scale.y;
	mat[6] = (cx*sy*sz + sx*cz)*scale.y;
	mat[7] = 0;

	mat[8] = (sy)*scale.z;
	mat[9] = (-sx*cy)*scale.z;
	mat[10] = (cx*cy)*scale.z;
	mat[11] = 0;

	mat[12] = pos.x;
	mat[13] = pos.y;
	mat[14] = pos.z;
	mat[15] = 1;
}
