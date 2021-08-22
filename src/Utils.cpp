#include "Utils.h"

#include <GLFW/glfw3.h>

namespace MathUtils {
Vec3i::Vec3i() {
	x = 0;
	y = 0;
	z = 0;
}

Vec3i::Vec3i(int a_x, int a_y, int a_z) {
	x = a_x;
	y = a_y;
	z = a_z;
}

Vec3i Vec3i::add(Vec3i input) {
	return Vec3i(x + input.x, y + input.y , z + input.z);
}

inline float Vec3i::dist(Vec3i other) {
	float xd = x - other.x;
	float yd = y - other.y;
	float zd = z - other.z;
	return sqrt(xd*xd + yd*yd + zd*zd);
}

std::string Vec3i::to_string() {
	return "(" + std::to_string(x) + ", "+ std::to_string(y) + ", "+ std::to_string(z) + ")";
}

Vec3::Vec3() {
	x = 0;
	y = 0;
	z = 0;
}

Vec3::Vec3(float a_x, float a_y, float a_z) {
	x = a_x;
	y = a_y;
	z = a_z;
}

Vec3 Vec3::add(Vec3 input) {
	return Vec3(x + input.x, y + input.y , z + input.z );
}

Vec3i Vec3::addi(Vec3 input) {
	return Vec3i((int)(x + input.x), (int)(y + input.y) , (int)(z + input.z));
}

inline float Vec3::dist(Vec3 other) {
	float xd = x - other.x;
	float yd = y - other.y;
	float zd = z - other.z;
	return sqrt(xd*xd + yd*yd + zd*zd);
}

float* getPerspective(float aspect, float fov) {
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

Vec3i8 Vec3i8::add(Vec3i8 input) {
	return { (uint8_t)(x + input.x),  (uint8_t)(y + input.y) ,  (uint8_t)(z + input.z) };
}

Vec3 Vec3i8::add(Vec3 input) {
	return Vec3(x + input.x, y + input.y , z + input.z );
}

void getWorldview(float* mat, Vec3 pos, Vec3 rot) {
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

void getModelview(float* mat, Vec3 pos, Vec3 rot, Vec3 scale) {
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
}

Timer::Timer(const std::string& name) {
	m_startTime = -1;
	m_name = name;
	
	reset();
}

void Timer::reset() {
	m_count = 0;
	m_time = 0;
}

void Timer::start() {
	m_startTime = glfwGetTime();
}

void Timer::stop() {
	m_time = m_time + glfwGetTime() - m_startTime;
	m_count++;
}

double Timer::getLapTime() {
	return m_time/m_count;
}

const std::string& Timer::getName() {
	return m_name;
}

void Timer::printLapTime() {
	printf("%s: %g ms\n", m_name.c_str(), getLapTime() * 1000);
}

void Timer::printLapTimeAndFPS() {
	printf("%s: %g ms (%g FPS)\n", m_name.c_str(), getLapTime() * 1000, 1.0 / getLapTime());
}

namespace FileUtils {
std::string readTextFile(std::string path) {
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

char* readBinaryFile(const char *filename, size_t* size) {
	char *buffer = NULL;
	size_t string_size, read_size;

	std::string f_name;

//#ifdef _DEBUG
#ifdef UTIL_DIR_PRE
	f_name.append(UTIL_DIR_PRE);
#endif
//#endif

	f_name.append(filename);

	FILE *handler = fopen(f_name.c_str(), "rb");

	if (handler) {
		fseek(handler, 0, SEEK_END);
		string_size = ftell(handler);
		rewind(handler);

		buffer = (char*)malloc(sizeof(char) * (string_size + 1));

		read_size = fread(buffer, sizeof(char), string_size, handler);

		buffer[string_size] = '\0';

		if (string_size != read_size) {
			printf("Error occured while reading file!\nstring_size = %zu\nread_size = %zu\n\n", string_size, read_size);
			free(buffer);
			buffer = NULL;
		}

		*size = read_size;

		fclose(handler);
	} else {
		printf("Did not find file %s!\n", filename);
	}

	return buffer;
}
}
