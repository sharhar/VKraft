#include <VKL/VKL.h>
#include <GLFW/glfw3.h>

#include <iostream>

#include "BG.h"
#include "Camera.h"
#include "ChunkRenderer.h"
#include "ChunkManager.h"
#include "Cursor.h"
#include "TextObject.h"

#include <stdio.h>

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
	
	VkExtent2D windowSize;
	windowSize.width = 800;
	windowSize.height = 600;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	GLFWwindow* window = glfwCreateWindow(windowSize.width, windowSize.height, "VKraft", NULL, NULL);

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//glfwSetWindowFocusCallback(window, window_focus_callback);
	//glfwSetMouseButtonCallback(window, mouse_button_callback);

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	
	VKLInstance instance(VKLInstanceCreateInfo()
						.procAddr(glfwGetInstanceProcAddress)
						.addExtensions(glfwExtensions, glfwExtensionCount)
						.debug(VK_TRUE));
	
	VkSurfaceKHR surface = VK_NULL_HANDLE;
	glfwCreateWindowSurface(instance.handle(), window, NULL, &surface);
	
	const VKLPhysicalDevice& physicalDevice = instance.getPhysicalDevices()[0];
	
	for (int i = 0; i < physicalDevice.getQueueFamilyProperties().size(); i++) {
		printf("%d: %d (%d)\n", i, physicalDevice.getQueueFamilyProperties()[i].queueFlags, physicalDevice.getQueueFamilyProperties()[i].queueCount);
	}
	
	VKLDevice device(VKLDeviceCreateInfo()
						.physicalDevice(&physicalDevice)
						.addExtension("VK_KHR_swapchain")
						.queueTypeCount(VKL_QUEUE_TYPE_GRAPHICS, 1)
						.queueTypeCount(VKL_QUEUE_TYPE_COMPUTE, 1)
						.queueTypeCount(VKL_QUEUE_TYPE_TRANSFER, 1));

	VKLQueue graphicsQueue = device.getQueue(VKL_QUEUE_TYPE_GRAPHICS, 0);
	VKLQueue computeQueue = device.getQueue(VKL_QUEUE_TYPE_COMPUTE, 0);
	VKLQueue transferQueue = device.getQueue(VKL_QUEUE_TYPE_TRANSFER, 0);
	
	VKLSwapChain swapChain(VKLSwapChainCreateInfo()
							.queue(&graphicsQueue)
							.surface(surface)
							.presentMode(VK_PRESENT_MODE_IMMEDIATE_KHR));

	VKLRenderPass renderPass(VKLRenderPassCreateInfo().device(&device)
								.addAttachment(VK_FORMAT_R16G16B16A16_SFLOAT).end()
								.addSubpass()
									.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
								.end());
	
	VkClearValue clearColor;
	clearColor.color.float32[0] = 0.25f;
	clearColor.color.float32[1] = 0.45f;
	clearColor.color.float32[2] = 1.0f;
	clearColor.color.float32[3] = 1.0f;
	
	swapChain.setClearValue(clearColor, 0);
	
	Cursor cursor(&device, &swapChain, &graphicsQueue);
	
	VKLCommandBuffer cmdBuffer(&graphicsQueue);
	
	VkFence renderFence = device.createFence(0);
	
	Timer frameTime("FrameTime");

	TextObject::init(&device, &transferQueue, &swapChain);

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
	
	while (!glfwWindowShouldClose(window)) {
		frameTime.start();
		
		glfwPollEvents();
		
		dt = glfwGetTime() - ct;
		ct = glfwGetTime();

		//if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		//	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		//}

		accDT += dt;
		frames++;

		if (accDT > 1) {
			fps = frames;
			
			std::cout << ("FPS:" + std::to_string((int) (1.0 / frameTime.getLapTime()))) << std::endl;
			
			//fpsText->setText("FPS:" + std::to_string((int) (1.0 / frameTime.getLapTime())));
			
			frameTime.reset();
			
			frames = 0;
			accDT = 0;
		}
		
		cmdBuffer.begin();
		
		swapChain.beingRender(&cmdBuffer);
		
		cursor.render(&cmdBuffer);
		
		swapChain.endRender(&cmdBuffer);
		
		cmdBuffer.end();
		
		graphicsQueue.submit(&cmdBuffer, renderFence);
		
		device.waitForFence(renderFence);
		device.resetFence(renderFence);

		swapChain.present();
		
		frameTime.stop();
	}
	
	TextObject::destroy();

	device.destroyFence(renderFence);
	
	cmdBuffer.destroy();
	
	renderPass.destroy();

	swapChain.destroy();
	cursor.destroy();
	device.destroy();
	
	instance.destroySurface(surface);
	instance.destroy();
	
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

/*
VkAttachmentDescription passAttachments[2];
	memset(passAttachments, 0, sizeof(VkAttachmentDescription) * 2);
	passAttachments[0].format = createInfo.m_createInfo.imageFormat;
	passAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	passAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	passAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	passAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	passAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	passAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	passAttachments[1].format = VK_FORMAT_D32_SFLOAT;
	passAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	passAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	passAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	passAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	passAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentReference;
	memset(&colorAttachmentReference, 0, sizeof(VkAttachmentReference));
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference;
	memset(&depthAttachmentReference, 0, sizeof(VkAttachmentReference));
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass;
	memset(&subpass, 0, sizeof(VkSubpassDescription));
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pDepthStencilAttachment = NULL;//&depthAttachmentReference;
*/

/*

float msaaAmount = 1.4f;

VKLFrameBuffer* msaaBuffer;
vklCreateFrameBuffer(device->deviceGraphicsContexts[0], &msaaBuffer, swapChain->width * msaaAmount, swapChain->height * msaaAmount, VK_FORMAT_R8G8B8A8_UNORM, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

ChunkRenderer::init(device, msaaBuffer);
Camera::init(window);

for(int x = -6; x < 6; x++) {
	for(int z = -6; z < 6; z++) {
		for(int y = -4; y < 4; y++) {
			ChunkManager::addChunk(Vec3i(x, y, z));
		}
	}
}

BG::init(device, swapChain, msaaBuffer);
Cursor::init(device, swapChain, msaaBuffer);
TextObject::init(device, msaaBuffer);

VkCommandBuffer cmdBuffer;
vklAllocateCommandBuffer(device->deviceGraphicsContexts[0], &cmdBuffer, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

vklSetClearColor(msaaBuffer, 0.25f, 0.45f, 1.0f, 1.0f );
vklSetClearColor(backBuffer, 1.0f, 0.0f, 1.0f, 1.0f );

TextObject* fpsText = new TextObject(16);
TextObject* rpsText = new TextObject(16);
TextObject* posText = new TextObject(64);

fpsText->setCoords(20, 20, 42);
fpsText->setText("FPS:0");

rpsText->setCoords(20, 68, 42);
rpsText->setText("RPS:0");

posText->setCoords(20, 116, 32);
posText->setText("POS: 100, 100, 100");
=======
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
>>>>>>> 59ba778313f8d601a5965c07913de1dc2b868b72=

		frameTime.start();
		
		renderTime.start();
		
		
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
	
delete fpsText;
delete rpsText;
delete posText;

BG::destroy();
Cursor::destroy();
TextObject::destroy();
ChunkRenderer::destroy();
Camera::destroy();
*/
