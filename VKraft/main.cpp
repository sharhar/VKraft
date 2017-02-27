#include "master.h"
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

	VLKShader shader = vlkCreateShader(device, "vert.spv", "geom.spv", "frag.spv", &uniformBuffer, sizeof(CubeUniformBuffer));
	VLKPipeline pipeline = vlkCreatePipeline(device, swapChain, shader);

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

	Camera::init(window, uniformBuffer.view);

	VulkanRenderContext renderContext = {};
	renderContext.device = device;
	renderContext.swapChain = swapChain;
	renderContext.uniformBuffer = &uniformBuffer;
	renderContext.shader = shader;
	renderContext.pipeline = pipeline;

	Cube::init();
	Chunk::init(245325, window, &renderContext);

	VLKTexture texture = vlkCreateTexture(device, "pack.png");
	vlkBindTexture(device, shader, texture);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		dts = glfwGetTime() - cts;

		std::this_thread::sleep_for(std::chrono::nanoseconds((long)((1.0f/120.0f - dts - 0.001) * 1000000000L)));

		cts = glfwGetTime();

		dt = glfwGetTime() - ct;
		ct = glfwGetTime();

		Camera::update(dt);

		vlkClear(context, device, swapChain);

		Chunk::render(device, swapChain);
		
		vlkSwap(context, device, swapChain);
	}

	Chunk::destroy(device);

	vlkDestroyShader(device, shader);
	vlkDestroyDeviceandSwapChain(context, device, swapChain);
	vlkDestroyContext(context);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}