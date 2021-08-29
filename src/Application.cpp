#include "Application.h"
#include "Cursor.h"
#include "TextObject.h"
#include "Camera.h"
#include "ChunkManager.h"
#include "ChunkRenderer.h"

#include <string>

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

Application::Application(int width, int height, const char* title) {
	glfwInit();

	winWidth = width;
	winHeight = height;
	
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(width, height, "VKraft", NULL, NULL);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowFocusCallback(window, window_focus_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	
	instance.create(VKLInstanceCreateInfo()
						.procAddr(glfwGetInstanceProcAddress)
						.addExtensions(glfwExtensions, glfwExtensionCount)
						.debug(VK_TRUE));
	
	VkSurfaceKHR surfaceHandle;
	glfwCreateWindowSurface(instance.handle(), window, NULL, &surfaceHandle);
	
	surface.create(VKLSurfaceCreateInfoHandle().instance(&instance).handle(surfaceHandle));
	
	device.create(VKLDeviceCreateInfo()
						.physicalDevice(&instance.getPhysicalDevices()[0])
						.addExtension("VK_KHR_swapchain")
						.queueTypeCount(VKL_QUEUE_TYPE_GRAPHICS, 1)
						.queueTypeCount(VKL_QUEUE_TYPE_COMPUTE, 1)
						.queueTypeCount(VKL_QUEUE_TYPE_TRANSFER, 1));
	
	graphicsQueue = device.getQueue(VKL_QUEUE_TYPE_GRAPHICS, 0);
	computeQueue = device.getQueue(VKL_QUEUE_TYPE_COMPUTE, 0);
	transferQueue = device.getQueue(VKL_QUEUE_TYPE_TRANSFER, 0);
	
	swapChain.create(VKLSwapChainCreateInfo()
						.queue(graphicsQueue)
						.surface(&surface)
						.presentMode(VK_PRESENT_MODE_IMMEDIATE_KHR));

	renderPass.create(VKLRenderPassCreateInfo().device(&device)
						.addAttachment(VK_FORMAT_R16G16B16A16_SFLOAT) // actual back buffer
							.layout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
						.end()
						.addAttachment(VK_FORMAT_R16G16B16A16_SFLOAT).end() // temp color ref for post-processing
						.addAttachment(VK_FORMAT_D32_SFLOAT)
							.layout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
							.op(VK_ATTACHMENT_LOAD_OP_CLEAR, VK_ATTACHMENT_STORE_OP_DONT_CARE)
						.end()
						.addSubpass()
							.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
							.addColorAttachment(1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
							.depthAttachment(2, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
						.end()
						.addSubpass()
							.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
							.addInputAttachment(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
						.end()
						.addSubpassDependency(0, 1, VK_DEPENDENCY_BY_REGION_BIT)
							.access(VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT)
							.stages(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
						.end());
	
	backBuffersCreateInfo.device(&device).memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY);
	
	cmdBuffer = new VKLCommandBuffer(graphicsQueue);
	
	cursor = new Cursor(this);
	camera = new Camera(this);
	
	chunkManager = new ChunkManager(this);
	
	for(int x = -6; x < 6; x++) {
		printf("x: %d\n", x);
		for(int z = -6; z < 6; z++) {
			for(int y = -4; y < 4; y++) {
				chunkManager->addChunk(MathUtils::Vec3i(x, y, z));
			}
		}
	}

	chunkRenderer = new ChunkRenderer(this);
	
	createBackBuffer(width, height);
	
	setupTextRenderingData();
	
	fpsText = new TextObject(this, 25);
	
	fpsText->setText("Hello World!");
	fpsText->setCoords(20, 20, 32);
}

void Application::createBackBuffer(uint32_t width, uint32_t height) {
	backBuffersCreateInfo.extent(width, height, 1);
	
	backBuffersCreateInfo.usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT).format(VK_FORMAT_R16G16B16A16_SFLOAT);
	
	backBuffers[0].create(backBuffersCreateInfo);
	
	backBuffersCreateInfo.usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
	
	backBuffers[1].create(backBuffersCreateInfo);
	
	backBuffersCreateInfo.usage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT).format(VK_FORMAT_D32_SFLOAT);
	
	backBuffers[2].m_aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
	backBuffers[2].create(backBuffersCreateInfo);
	
	transferQueue->getCmdBuffer()->begin();
	
	backBuffers[0].cmdTransitionBarrier(transferQueue->getCmdBuffer(), VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
									VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	
	backBuffers[1].cmdTransitionBarrier(transferQueue->getCmdBuffer(), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	
	backBuffers[2].cmdTransitionBarrier(transferQueue->getCmdBuffer(), VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
										VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
	
	transferQueue->getCmdBuffer()->end();
	
	transferQueue->submitAndWait(transferQueue->getCmdBuffer());
	
	backBufferViews[0].create(VKLImageViewCreateInfo().image(&backBuffers[0]));
	backBufferViews[1].create(VKLImageViewCreateInfo().image(&backBuffers[1]));
	backBufferViews[2].create(VKLImageViewCreateInfo().image(&backBuffers[2]));
	
	framebuffer.create(VKLFramebufferCreateInfo()
							.renderPass(&renderPass)
							.addAttachment(&backBufferViews[0])
							.addAttachment(&backBufferViews[1])
							.addAttachment(&backBufferViews[2])
							.extent(width, height, 1));
	
	VkClearValue clearColor;
	clearColor.color.float32[0] = 0.25f;
	clearColor.color.float32[1] = 0.45f;
	clearColor.color.float32[2] = 1.0f;
	clearColor.color.float32[3] = 1.0f;
	
	framebuffer.setClearValue(clearColor, 0);
	framebuffer.setClearValue(clearColor, 1);
	
	VkClearValue depthColor;
	depthColor.depthStencil.depth = 1.0f;
	depthColor.depthStencil.stencil = 0;
	
	framebuffer.setClearValue(depthColor, 2);
	
	cursor->bindInputAttachment(&backBufferViews[1]);
}

void Application::mainLoop() {
	double ct = glfwGetTime();
	double dt = ct;
	
	while (!glfwWindowShouldClose(window)) {
		dt = glfwGetTime() - ct;
		ct = glfwGetTime();
		
		camera->update(dt);
		
		pollWindowEvents();
		render();
	}
}


void Application::pollWindowEvents() {
	glfwPollEvents();
	
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
	
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	
	if(winWidth != width || winHeight != height) {
		winWidth = width;
		winHeight = height;
		
		framebuffer.destroy();
		
		backBufferViews[0].destroy();
		backBufferViews[1].destroy();
		backBufferViews[2].destroy();
		
		backBuffers[0].destroy();
		backBuffers[1].destroy();
		backBuffers[2].destroy();
		
		createBackBuffer(width, height);
		
		swapChain.rebuild();
	}
}

void Application::render() {
	cmdBuffer->begin();
	
	cmdBuffer->beginRenderPass(framebuffer, VK_SUBPASS_CONTENTS_INLINE);
	
	cmdBuffer->setViewPort(0, winWidth, winHeight);
	cmdBuffer->setViewPort(1, winWidth, winHeight);
	
	cmdBuffer->setScissor(0, winWidth, winHeight);
	//cmdBuffer->setScissor(1, winWidth, winHeight);
	//cmdBuffer->setScissor(1, 0, 0, 0, 0);
	
	chunkRenderer->render(cmdBuffer);
	perpareTextRendering(cmdBuffer);
	fpsText->render(cmdBuffer);
	
	cmdBuffer->nextSubpass(VK_SUBPASS_CONTENTS_INLINE);
	cmdBuffer->setScissor(0, winWidth, winHeight);
	cmdBuffer->setScissor(1, winWidth, winHeight);
	cursor->render(cmdBuffer);
	cmdBuffer->endRenderPass();
	
	cmdBuffer->end();
	
	graphicsQueue->submitAndWait(cmdBuffer);

	swapChain.present(&backBuffers[0]);
}

void Application::destroy() {
	fpsText->destroy();
	delete fpsText;
	
	cleanUpTextRenderingData();
	
	chunkRenderer->destroy();
	delete chunkRenderer;
	
	chunkManager->destroy();
	delete chunkManager;
	
	camera->destroy();
	delete camera;
	
	cursor->destroy();
	delete cursor;
	
	cmdBuffer->destroy();
	delete cmdBuffer;
	
	framebuffer.destroy();
	
	backBufferViews[0].destroy();
	backBufferViews[1].destroy();
	backBufferViews[2].destroy();
	
	backBuffers[0].destroy();
	backBuffers[1].destroy();
	backBuffers[2].destroy();
	
	renderPass.destroy();
	swapChain.destroy();
	
	device.destroy();
	surface.destroy();
	instance.destroy();
	
	glfwDestroyWindow(window);
	glfwTerminate();
}
