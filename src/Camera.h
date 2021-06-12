//
//  Camera.hpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/12/21.
//

#ifndef Camera_h
#define Camera_h

#include <GLFW/glfw3.h>
#include <VKL/VKL.h>
#include "Utils.h"

#include <thread>
#include <chrono>

class Camera {
public:
	static void init(GLFWwindow* window);
	static void update(float dt);
	static void destroy();
private:
	static double prev_x, prev_y;
	static float yVel;
	static Vec3 pos;
	static Vec3 renderPos;
	static Vec3 rot;
	static GLFWwindow* m_window;
};

#endif /* Camera_h */
