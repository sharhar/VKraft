#include "VLKUtils.h"
#include "Cube.h"
#include "Utils.h"
#include <chrono>
#include <thread>

static void window_focus_callback(GLFWwindow* window, int focused) {
	if (focused) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (action == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "VKraft", NULL, NULL);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowFocusCallback(window, window_focus_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	VLKContext context = vlkCreateContext();
	VLKDevice device;
	VLKSwapchain swapChain;
	vlkCreateDeviceAndSwapchain(window, context, device, swapChain);

	CubeUniformBuffer uniformBuffer;

	memcpy(uniformBuffer.proj, getPerspective(), sizeof(float) * 16);

	Vec3 pos = {0, 0, 1};
	Vec3 rot = {0, 0, 0};
	double prev_x = 0;
	double prev_y = 0;

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	prev_x = xpos;
	prev_y = ypos;

	double ct = glfwGetTime();
	double dt = 0;

	double cts = glfwGetTime();
	double dts = 0;

	PlayerInfo* playerInfo = (PlayerInfo*)malloc(sizeof(PlayerInfo));
	playerInfo->pos = {0, 0, 0};

	Cube::init();
	Chunk::init(245325, window, device, swapChain, &uniformBuffer, playerInfo);
	
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		dts = glfwGetTime() - cts;

		std::this_thread::sleep_for(std::chrono::nanoseconds((long)((1.0f/120.0f - dts - 0.001) * 1000000000L)));

		cts = glfwGetTime();

		dt = glfwGetTime() - ct;
		ct = glfwGetTime();

		bool focused = glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		if (focused) {
			float speed = 3.5f;

			if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
				speed *= 2;
			}

			float xVel = 0;
			float yVel = 0;
			float zVel = 0;

			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
				zVel -= speed * cos(rot.y*DEG_TO_RAD) * dt;
				xVel += speed * sin(rot.y*DEG_TO_RAD) * dt;
			}

			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
				zVel += speed * cos(rot.y*DEG_TO_RAD) * dt;
				xVel -= speed * sin(rot.y*DEG_TO_RAD) * dt;
			}

			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
				zVel -= speed * sin(rot.y*DEG_TO_RAD) * dt;
				xVel -= speed * cos(rot.y*DEG_TO_RAD) * dt;
			}

			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
				zVel += speed * sin(rot.y*DEG_TO_RAD) * dt;
				xVel += speed * cos(rot.y*DEG_TO_RAD) * dt;
			}

			if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
				yVel += speed * dt;
			}

			if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
				yVel -= speed * dt;
			}

			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			}

			pos.x += xVel;
			pos.y -= yVel;
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

		playerInfo->pos = pos;

		getWorldview(uniformBuffer.view, pos, rot);

		vlkClear(context, device, swapChain);

		Chunk::render(device, swapChain);
		
		vlkSwap(context, device, swapChain);
	}

	Chunk::destroy(device);

	vlkDestroyDeviceandSwapChain(context, device, swapChain);
	vlkDestroyContext(context);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}