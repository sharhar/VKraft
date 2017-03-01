#pragma once

#include "VLKUtils.h"
#include "Utils.h"
#include <thread>

class Camera {
private:
	static double prev_x, prev_y;
	static std::thread* cameraThread;
public:
	static int fence;
	static bool grounded;
	static float* worldviewMat;
	static GLFWwindow* window;

	static Vec3* poss;
	static int possSize;

	static float yVel;
	static Vec3 pos;
	static Vec3 renderPos;
	static Vec3 rot;

	static void init(GLFWwindow* win, float* viewMat, VulkanRenderContext* vrc);
	static void update(float dt);
	static void destroy();
};