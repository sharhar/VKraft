#include "Application.h"
#include "Camera.h"
#include "ChunkRenderer.h"

Camera::Camera(Application* application) {
	m_application = application;
	
	m_pos = MathUtils::Vec3(0, 8, -3);
	m_renderPos = m_pos;
	m_rot = MathUtils::Vec3(0, 90, 0);
	
	double xpos, ypos;
	glfwGetCursorPos(m_application->window, &xpos, &ypos);

	m_prev_x = xpos;
	m_prev_y = ypos;
	
	m_yVel = 0;
}

void Camera::update(float dt) {
	bool focused = glfwGetInputMode(m_application->window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

	double xpos, ypos;
	glfwGetCursorPos(m_application->window, &xpos, &ypos);

	if (focused) {
		float speed = 3.5f;

		if (glfwGetKey(m_application->window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
			speed *= 2;
		}

		float xVel = 0;
		float zVel = 0;

		if (glfwGetKey(m_application->window, GLFW_KEY_W) == GLFW_PRESS) {
			zVel -= speed * cos(m_rot.y*DEG_TO_RAD) * dt;
			xVel += speed * sin(m_rot.y*DEG_TO_RAD) * dt;
		}

		if (glfwGetKey(m_application->window, GLFW_KEY_S) == GLFW_PRESS) {
			zVel += speed * cos(m_rot.y*DEG_TO_RAD) * dt;
			xVel -= speed * sin(m_rot.y*DEG_TO_RAD) * dt;
		}

		if (glfwGetKey(m_application->window, GLFW_KEY_A) == GLFW_PRESS) {
			zVel -= speed * sin(m_rot.y*DEG_TO_RAD) * dt;
			xVel -= speed * cos(m_rot.y*DEG_TO_RAD) * dt;
		}

		if (glfwGetKey(m_application->window, GLFW_KEY_D) == GLFW_PRESS) {
			zVel += speed * sin(m_rot.y*DEG_TO_RAD) * dt;
			xVel += speed * cos(m_rot.y*DEG_TO_RAD) * dt;
		}
		
		if (glfwGetKey(m_application->window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			m_pos.y += speed * dt;
		}
		if (glfwGetKey(m_application->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			m_pos.y -= speed * dt;
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
		m_pos.x += xVel;
		m_pos.z += zVel;
		
		float xdiff = (m_prev_x - xpos);

		if (xdiff != 0) {
			m_rot.y -= 5 * xdiff * 0.085f;// *2 * dt;
		}

		float ydiff = (m_prev_y - ypos);

		if (ydiff != 0) {
			m_rot.x += 5 * ydiff * 0.085f;// *2 * dt;
		}

		if (m_rot.x > 90) {
			m_rot.x = 90;
		}

		if (m_rot.x < -90) {
			m_rot.x = -90;
		}
	}

	m_prev_x = xpos;
	m_prev_y = ypos;

	m_renderPos.x = m_pos.x;
	m_renderPos.y = -m_pos.y;
	m_renderPos.z = m_pos.z;

	MathUtils::getWorldview(m_application->chunkRenderer->getUniform()->view, m_renderPos, m_rot);
}

void Camera::destroy() {
	
}
