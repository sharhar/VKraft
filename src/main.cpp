#include <VKL/VKL.h>
#include <GLFW/glfw3.h>

//#include "VLKUtils.h"
//#include "Cube.h"
//#include "Camera.h"

#include "BG.h"
#include "Camera.h"
#include "ChunkManager.h"
#include "Cursor.h"
#include "FontEngine.h"

void window_focus_callback(GLFWwindow* window, int focused) {
	if (focused) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
	else {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (action == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}
}

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "VKraft", NULL, NULL);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowFocusCallback(window, window_focus_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	VkBool32 debug = 0;

#ifdef _DEBUG
	debug = 1;
#endif

	uint32_t extensionCountGLFW = 0;
	char** extensionsGLFW = (char**)glfwGetRequiredInstanceExtensions(&extensionCountGLFW);

	VKLInstance* instance;
	vklCreateInstance(&instance, NULL, extensionsGLFW[1], glfwGetInstanceProcAddress, debug);

	VKLSurface* surface = (VKLSurface*) malloc(sizeof(VKLSurface));
	glfwCreateWindowSurface(instance->instance, window, NULL, &surface->surface);

	surface->width = 1280;
	surface->height = 720;

	VKLDevice* device;
	VKLDeviceGraphicsContext** deviceContexts;
	VKLDeviceComputeContext** deviceComps;
	vklCreateDevice(instance, &device, &surface, 1, &deviceContexts, 1, &deviceComps);

	VKLSwapChain* swapChain;
	VKLFrameBuffer* backBuffer;
	vklCreateSwapChain(device->deviceGraphicsContexts[0], &swapChain, VK_TRUE);
	vklGetBackBuffer(swapChain, &backBuffer);
	
	ChunkManager::init(device, swapChain->width * 2, swapChain->height * 2);
	Camera::init(window);
	
	BG::init(device, swapChain, ChunkManager::getFramebuffer());
	Cursor::init(device, swapChain, ChunkManager::getFramebuffer());
	FontEngine::init(device, swapChain);

	//Camera::init(window, &uniformBuffer, context);
	
	
	
	double ct = glfwGetTime();
	double dt = ct;

	float accDT = 0;
	uint32_t frames = 0;
	uint32_t fps = 0;

	int width, height;
	int pwidth, pheight;

	glfwGetWindowSize(window, &width, &height);
	pwidth = width;
	pheight = height;
	
	VkCommandBuffer cmdBuffer;
	vklAllocateCommandBuffer(device->deviceGraphicsContexts[0], &cmdBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
	
	vklSetClearColor(ChunkManager::getFramebuffer(), 0.25f, 0.45f, 1.0f, 1.0f );
	vklSetClearColor(backBuffer, 1.0f, 0.0f, 1.0f, 1.0f );
	
	FontEngine::setCoords(10, 10, 16);
	FontEngine::setText("FPS:0");

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		
		glfwGetWindowSize(window, &width, &height);

		if (pwidth != width || pheight != height) {
			//vlkRecreateSwapchain(device, &swapChain, false);
			
			Cursor::updateProjection(width, height);
			FontEngine::updateProjection(width, height);
		}
		
		pwidth = width;
		pheight = height;
		
		dt = glfwGetTime() - ct;
		ct = glfwGetTime();

		accDT += dt;
		frames++;

		if (accDT > 1) {
			fps = frames;
			FontEngine::setText("FPS:" + std::to_string(fps));
			frames = 0;
			accDT = 0;
		}
		
		Camera::update(dt);
		
		vklBeginCommandBuffer(device, cmdBuffer);
		
		ChunkManager::render(cmdBuffer);
		
		vklBeginRender(device, backBuffer, cmdBuffer);
		
		VkViewport viewport = { 0, 0, swapChain->width, swapChain->height, 0, 1 };
		VkRect2D scissor = { {0, 0}, {swapChain->width, swapChain->height} };
		
		device->pvkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		device->pvkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
		
		BG::render(cmdBuffer);
		Cursor::render(cmdBuffer);
		FontEngine::render(cmdBuffer);
		
		vklEndRender(device, backBuffer, cmdBuffer);
		
		vklEndCommandBuffer(device, cmdBuffer);
		
		vklExecuteCommandBuffer(device->deviceGraphicsContexts[0], cmdBuffer);

		vklPresent(swapChain);
	}
	
	BG::destroy();
	Cursor::destroy();
	FontEngine::destroy();
	ChunkManager::destroy();
	Camera::destroy();
	
	vklDestroySwapChain(swapChain);
	vklDestroyDevice(device);
	vklDestroySurface(instance, surface);
	vklDestroyInstance(instance);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
