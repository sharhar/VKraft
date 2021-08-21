#include <VKL/VKL.h>

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

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
	
	VkSurfaceKHR surfaceHandle;
	glfwCreateWindowSurface(instance.handle(), window, NULL, &surfaceHandle);
	
	VKLSurface surface(VKLSurfaceCreateInfoHandle().instance(&instance).handle(surfaceHandle));
	
	VKLDevice device(VKLDeviceCreateInfo()
						.physicalDevice(&instance.getPhysicalDevices()[0])
						.addExtension("VK_KHR_swapchain")
						.queueTypeCount(VKL_QUEUE_TYPE_GRAPHICS, 1)
						.queueTypeCount(VKL_QUEUE_TYPE_COMPUTE, 1)
						.queueTypeCount(VKL_QUEUE_TYPE_TRANSFER, 1));

	VKLQueue graphicsQueue = device.getQueue(VKL_QUEUE_TYPE_GRAPHICS, 0);
	//VKLQueue computeQueue = device.getQueue(VKL_QUEUE_TYPE_COMPUTE, 0);
	VKLQueue transferQueue = device.getQueue(VKL_QUEUE_TYPE_TRANSFER, 0);
	
	VKLSwapChain swapChain(VKLSwapChainCreateInfo()
							.queue(&graphicsQueue)
							.surface(surface.handle())
							.presentMode(VK_PRESENT_MODE_IMMEDIATE_KHR));

	VKLRenderPass renderPass(VKLRenderPassCreateInfo().device(&device)
								.addAttachment(VK_FORMAT_R16G16B16A16_SFLOAT) // actual back buffer
									.layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
								.end()
								.addAttachment(VK_FORMAT_R16G16B16A16_SFLOAT).end() // temp color ref for post-processing
								.addSubpass()
									.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
									.addColorAttachment(1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
								.end()
								.addSubpass()
									.addInputAttachment(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
									.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
								.end()
								.addSubpassDependency(0, 1, VK_DEPENDENCY_BY_REGION_BIT)
									.access(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
									.stages(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
								.end());
	
	VKLImage backBuffer(VKLImageCreateInfo()
							.device(&device)
							.extent(800, 600, 1)
							.format(VK_FORMAT_R16G16B16A16_SFLOAT)
							.usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
							.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));
	
	VKLImage backBufferRef(VKLImageCreateInfo()
							.device(&device)
							.extent(800, 600, 1)
							.format(VK_FORMAT_R16G16B16A16_SFLOAT)
							.usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)
							.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));
	
	transferQueue.getCmdBuffer()->begin();
	
	backBuffer.cmdTransitionBarrier(transferQueue.getCmdBuffer(), VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	backBufferRef.cmdTransitionBarrier(transferQueue.getCmdBuffer(), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	
	transferQueue.getCmdBuffer()->end();
	
	transferQueue.submit(transferQueue.getCmdBuffer(), VK_NULL_HANDLE);
	transferQueue.waitIdle();
	
	VKLImageView backBufferView(VKLImageViewCreateInfo().image(&backBuffer));
	VKLImageView backBufferRefView(VKLImageViewCreateInfo().image(&backBufferRef));
	
	VkImageView tempViews[2];
	tempViews[0] = backBufferView.handle();
	tempViews[1] = backBufferRefView.handle();
	
	VkFramebufferCreateInfo frameBufferCreateInfo;
	memset(&frameBufferCreateInfo, 0, sizeof(VkFramebufferCreateInfo));
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.renderPass = renderPass.handle();
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = tempViews;
	frameBufferCreateInfo.width = 800;
	frameBufferCreateInfo.height = 600;
	frameBufferCreateInfo.layers = 1;
	
	VkFramebuffer frameBuffer;
	
	VK_CALL(device.vk.CreateFramebuffer(device.handle(), &frameBufferCreateInfo, device.allocationCallbacks(), &frameBuffer));
	
	VkClearValue clearColor[2];
	clearColor[0].color.float32[0] = 0.25f;
	clearColor[0].color.float32[1] = 0.45f;
	clearColor[0].color.float32[2] = 1.0f;
	clearColor[0].color.float32[3] = 1.0f;
	clearColor[1] = clearColor[0];
	
	Cursor cursor(&device, &renderPass, &graphicsQueue, backBufferRefView.handle());
	
	VKLCommandBuffer cmdBuffer(&graphicsQueue);

	TextObject::init(&device, &transferQueue, &renderPass);

	int width, height;
	int pwidth, pheight;

	glfwGetWindowSize(window, &width, &height);
	pwidth = width;
	pheight = height;
	
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		//if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		//	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		//}
		
		cmdBuffer.begin();
		
		VkRect2D area;
		area.offset.x = 0;
		area.offset.y = 0;
		area.extent.width = 800;
		area.extent.height = 600;
		
		VkRenderPassBeginInfo renderPassBeginInfo;
		memset(&renderPassBeginInfo, 0, sizeof(VkRenderPassBeginInfo));
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass.handle();
		renderPassBeginInfo.framebuffer = frameBuffer;
		renderPassBeginInfo.renderArea = area;
		renderPassBeginInfo.clearValueCount = 2;
		renderPassBeginInfo.pClearValues = clearColor;
		device.vk.CmdBeginRenderPass(cmdBuffer.handle(), &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		
		device.vk.CmdNextSubpass(cmdBuffer.handle(), VK_SUBPASS_CONTENTS_INLINE);
		
		cursor.render(&cmdBuffer);
		
		device.vk.CmdEndRenderPass(cmdBuffer.handle());
		
		cmdBuffer.end();
		
		graphicsQueue.submit(&cmdBuffer, VK_NULL_HANDLE);
		graphicsQueue.waitIdle();

		swapChain.present(&backBuffer);
	}
	
	device.vk.DestroyFramebuffer(device.handle(), frameBuffer, device.allocationCallbacks());
	
	TextObject::destroy();

	cmdBuffer.destroy();
	
	backBufferRefView.destroy();
	backBufferRef.destroy();
	
	backBufferView.destroy();
	backBuffer.destroy();
	
	renderPass.destroy();

	swapChain.destroy();
	cursor.destroy();
	device.destroy();
	
	surface.destroy();
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
