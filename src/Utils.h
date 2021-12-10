#pragma once

#define DEG_TO_RAD 0.0174532925199

#include <iostream>
#include <fstream>
#include <string>
#include <math.h>

namespace MathUtils {
typedef struct Vec3i {
	int x, y, z;
	
	Vec3i();
	Vec3i(int a_x, int a_y, int a_z);

	Vec3i add(Vec3i input);
	inline float dist(Vec3i other);
	
	std::string to_string();
} Vec3i;

typedef struct Vec3 {
	float x, y, z;
	
	Vec3();
	Vec3(float a_x, float a_y, float a_z);

	Vec3 add(Vec3 input);
	Vec3i addi(Vec3 input);

	inline float dist(Vec3 other);
} Vec3;

typedef struct Vec3i8 {
	uint8_t x, y, z;

	Vec3i8 add(Vec3i8 input);
	Vec3 add(Vec3 input);
} Vec3i8;

float* getPerspective(float aspect, float fov);
void getWorldview(float* mat, Vec3 pos, Vec3 rot);
void getModelview(float* mat, Vec3 pos, Vec3 rot, Vec3 scale);
}

class Timer {
public:
	Timer(const std::string& name);
	
	void pause();
	void unpause();
	void reset();
	void start();
	void stop();
	
	double getLapTime();
	const std::string& getName();
	
	void printLapTime();
	void printLapTimeAndFPS();
private:
	double m_startTime;
	double m_time;
	double m_bufferTime;
	double m_startBuffer;
	int m_count;
	std::string m_name;
};

namespace FileUtils {
std::string readTextFile(std::string path);
char* readBinaryFile(const char *filename, size_t* size);
}



