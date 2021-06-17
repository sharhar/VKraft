#include <VKL/VKL.h>
#include <GLFW/glfw3.h>

#include "BG.h"
#include "Camera.h"
#include "ChunkRenderer.h"
#include "Cursor.h"
#include "TextObject.h"

class Timer {
private:
	double m_startTime;
	double m_time;
	int m_count;
	std::string m_name;
public:
	Timer(const std::string& name) {
		m_startTime = -1;
		m_name = name;
		
		reset();
	}
	
	void reset() {
		m_count = 0;
		m_time = 0;
	}
	
	void start() {
		m_startTime = glfwGetTime();
	}
	
	void stop() {
		m_time = m_time + glfwGetTime() - m_startTime;
		m_count++;
	}
	
	double getLapTime() {
		return m_time/m_count;
	}
	
	const std::string& getName() {
		return m_name;
	}
	
	void printLapTime() {
		printf("%s: %g ms\n", m_name.c_str(), getLapTime() * 1000);
	}
	
	void printLapTimeAndFPS() {
		printf("%s: %g ms (%g FPS)\n", m_name.c_str(), getLapTime() * 1000, 1.0 / getLapTime());
	}
};

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
	GLFWwindow* window = glfwCreateWindow(1600, 900, "VKraft", NULL, NULL);

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
	
	float msaaAmount = 1.4f;
	
	VKLFrameBuffer* msaaBuffer;
	vklCreateFrameBuffer(device->deviceGraphicsContexts[0], &msaaBuffer, swapChain->width * msaaAmount, swapChain->height * msaaAmount, VK_FORMAT_R8G8B8A8_UNORM, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	ChunkRenderer::init(device, msaaBuffer);
	Camera::init(window);
	
	ChunkRenderer::addChunk(Vec3i(0, -1, 0));
	ChunkRenderer::addChunk(Vec3i(0, -1, 1));
	ChunkRenderer::addChunk(Vec3i(0, -1, -1));
	
	ChunkRenderer::addChunk(Vec3i(1, -1, 0));
	ChunkRenderer::addChunk(Vec3i(1, -1, 1));
	ChunkRenderer::addChunk(Vec3i(1, -1, -1));
	
	ChunkRenderer::addChunk(Vec3i(-1, -1, 0));
	ChunkRenderer::addChunk(Vec3i(-1, -1, 1));
	ChunkRenderer::addChunk(Vec3i(-1, -1, -1));
	
	ChunkRenderer::addChunk(Vec3i(0, -2, 0));
	ChunkRenderer::addChunk(Vec3i(0, -2, 1));
	ChunkRenderer::addChunk(Vec3i(0, -2, -1));
	
	ChunkRenderer::addChunk(Vec3i(1, -2, 0));
	ChunkRenderer::addChunk(Vec3i(1, -2, 1));
	ChunkRenderer::addChunk(Vec3i(1, -2, -1));
	
	ChunkRenderer::addChunk(Vec3i(-1, -2, 0));
	ChunkRenderer::addChunk(Vec3i(-1, -2, 1));
	ChunkRenderer::addChunk(Vec3i(-1, -2, -1));
	
	ChunkRenderer::addChunk(Vec3i(0, -3, 0));
	ChunkRenderer::addChunk(Vec3i(0, -3, 1));
	ChunkRenderer::addChunk(Vec3i(0, -3, -1));
	
	ChunkRenderer::addChunk(Vec3i(1, -3, 0));
	ChunkRenderer::addChunk(Vec3i(1, -3, 1));
	ChunkRenderer::addChunk(Vec3i(1, -3, -1));
	
	ChunkRenderer::addChunk(Vec3i(-1, -3, 0));
	ChunkRenderer::addChunk(Vec3i(-1, -3, 1));
	ChunkRenderer::addChunk(Vec3i(-1, -3, -1));
	
	BG::init(device, swapChain, msaaBuffer);
	Cursor::init(device, swapChain, msaaBuffer);
	TextObject::init(device, msaaBuffer);
	
	TextObject* fpsText = new TextObject(16);
	TextObject* rpsText = new TextObject(16);
	TextObject* posText = new TextObject(64);
	
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
	
	vklSetClearColor(msaaBuffer, 0.25f, 0.45f, 1.0f, 1.0f );
	vklSetClearColor(backBuffer, 1.0f, 0.0f, 1.0f, 1.0f );
	
	fpsText->setCoords(20, 20, 42);
	fpsText->setText("FPS:0");
	
	rpsText->setCoords(20, 68, 42);
	rpsText->setText("RPS:0");
	
	posText->setCoords(20, 116, 32);
	posText->setText("POS: 100, 100, 100");
	
	Timer frameTime("FrameTime");
	Timer renderTime("RenderTime");

	while (!glfwWindowShouldClose(window)) {
		frameTime.start();
		
		renderTime.start();
		
		glfwPollEvents();

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
		
		glfwGetWindowSize(window, &width, &height);

		if (pwidth != width || pheight != height) {
			vklRecreateSwapChain(device->deviceGraphicsContexts[0], swapChain);
			backBuffer = swapChain->backBuffer;
			
			vklRecreateFrameBuffer(device->deviceGraphicsContexts[0], msaaBuffer, swapChain->width * msaaAmount, swapChain->height * msaaAmount);
			
			BG::rebuildPipeline(swapChain, msaaBuffer);
			Cursor::rebuildPipeline(swapChain, msaaBuffer);
			TextObject::rebuildPipeline();
			
			ChunkRenderer::rebuildPipeline();
			
			Cursor::updateProjection(width, height);
			fpsText->updateProjection();
			rpsText->updateProjection();
			posText->updateProjection();
		}
		
		pwidth = width;
		pheight = height;
		
		dt = glfwGetTime() - ct;
		ct = glfwGetTime();

		accDT += dt;
		frames++;

		if (accDT > 1) {
			fps = frames;
			
			fpsText->setText("FPS:" + std::to_string((int) (1.0 / frameTime.getLapTime())));
			rpsText->setText("RPS:" + std::to_string((int)(1.0 / renderTime.getLapTime())));
			
			frameTime.reset();
			renderTime.reset();
			
			frames = 0;
			accDT = 0;
		}
		
		Camera::update(dt);
		
		posText->setText("POS: " + std::to_string(Camera::pos.x) + ", " + std::to_string(Camera::pos.y) + ", " + std::to_string(Camera::pos.z));
		
		vklBeginCommandBuffer(device, cmdBuffer);
		
		vklBeginRender(device, msaaBuffer, cmdBuffer);
		
		ChunkRenderer::render(cmdBuffer);
		fpsText->render(cmdBuffer);
		rpsText->render(cmdBuffer);
		posText->render(cmdBuffer);
		
		vklEndRender(device, msaaBuffer, cmdBuffer);
		
		vklBeginRender(device, backBuffer, cmdBuffer);
		
		VkViewport viewport = { 0, 0, swapChain->width, swapChain->height, 0, 1 };
		VkRect2D scissor = { {0, 0}, {swapChain->width, swapChain->height} };
		
		device->pvkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		device->pvkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
		
		BG::render(cmdBuffer);
		Cursor::render(cmdBuffer);
		
		vklEndRender(device, backBuffer, cmdBuffer);
		
		vklEndCommandBuffer(device, cmdBuffer);
		
		vklExecuteCommandBuffer(device->deviceGraphicsContexts[0], cmdBuffer);
		
		renderTime.stop();

		vklPresent(swapChain);
		
		frameTime.stop();
	}
	
	delete fpsText;
	delete rpsText;
	
	BG::destroy();
	Cursor::destroy();
	TextObject::destroy();
	ChunkRenderer::destroy();
	Camera::destroy();
	
	vklDestroySwapChain(swapChain);
	vklDestroyDevice(device);
	vklDestroySurface(instance, surface);
	vklDestroyInstance(instance);

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
