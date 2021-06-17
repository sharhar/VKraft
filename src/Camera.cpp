//
//  Camera.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/12/21.
//

#include "Camera.h"
#include "ChunkRenderer.h"

Vec3 Camera::pos = Vec3(0, 8, -3);
Vec3 Camera::renderPos = Vec3(0, 0, 0);
Vec3 Camera::rot = Vec3(0, 90, 0);
double Camera::prev_x = 0;
double Camera::prev_y = 0;
float Camera::yVel = 0;
GLFWwindow* Camera::m_window = NULL;

void Camera::init(GLFWwindow *window) {
	m_window = window;
	
	renderPos.x = pos.x;
	renderPos.y = pos.y;
	renderPos.z = pos.z;
	
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	prev_x = xpos;
	prev_y = ypos;
}

void Camera::update(float dt) {
	bool focused = glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

	double xpos, ypos;
	glfwGetCursorPos(m_window, &xpos, &ypos);

	if (focused) {
		float speed = 3.5f;

		if (glfwGetKey(m_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
			speed *= 2;
		}

		float xVel = 0;
		float zVel = 0;

		if (glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS) {
			zVel -= speed * cos(rot.y*DEG_TO_RAD) * dt;
			xVel += speed * sin(rot.y*DEG_TO_RAD) * dt;
		}

		if (glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS) {
			zVel += speed * cos(rot.y*DEG_TO_RAD) * dt;
			xVel -= speed * sin(rot.y*DEG_TO_RAD) * dt;
		}

		if (glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS) {
			zVel -= speed * sin(rot.y*DEG_TO_RAD) * dt;
			xVel -= speed * cos(rot.y*DEG_TO_RAD) * dt;
		}

		if (glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS) {
			zVel += speed * sin(rot.y*DEG_TO_RAD) * dt;
			xVel += speed * cos(rot.y*DEG_TO_RAD) * dt;
		}
		
		if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			pos.y += speed * dt;
		}
		if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			pos.y -= speed * dt;
		}

		/*
		//T = 3/5, H = 1.2

		if (grounded && glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			yVel = 10;
			grounded = false;
		}

		yVel -= 33.3333f * dt;


		while (fence == 1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		fence = fence + 2;

		

		if (hittingCubes(pos)) {
			pos.x -= xVel;
		}

		pos.y += yVel * dt;

		if (hittingCubes(pos)) {
			pos.y -= yVel * dt;
			yVel = 0;
			grounded = true;
		}

		pos.z += zVel;

		if (hittingCubes(pos)) {
			pos.z -= zVel;
		}

		fence = fence - 2;
		 
*/
		pos.x += xVel;
		pos.z += zVel;
		
		float xdiff = (prev_x - xpos);

		if (xdiff != 0) {
			rot.y -= 5 * xdiff * 0.085f;// *2 * dt;
		}

		float ydiff = (prev_y - ypos);

		if (ydiff != 0) {
			rot.x += 5 * ydiff * 0.085f;// *2 * dt;
		}

		if (rot.x > 90) {
			rot.x = 90;
		}

		if (rot.x < -90) {
			rot.x = -90;
		}
	}

	prev_x = xpos;
	prev_y = ypos;

	renderPos.x = pos.x;
	renderPos.y = -pos.y;
	renderPos.z = pos.z;

	getWorldview(ChunkRenderer::getUniform()->view, renderPos, rot);
}

void Camera::destroy() {
	
}
