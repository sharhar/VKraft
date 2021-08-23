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

#include "Base.h"

#include <thread>
#include <chrono>

class Camera {
public:
	Camera(Application* application);
	void update(float dt);
	void destroy();
private:
	Application* m_application;
	
	double m_prev_x, m_prev_y;
	float m_yVel;
	MathUtils::Vec3 m_renderPos;
	MathUtils::Vec3 m_rot;
	MathUtils::Vec3 m_pos;
};

#endif /* Camera_h */
