#include "Application.h"
#include "Cursor.h"

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

Application::Application(uint32_t width, uint32_t height, const char* title) {
	glfwInit();
	
	VkExtent2D windowSize;
	windowSize.width = 800;
	windowSize.height = 600;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(windowSize.width, windowSize.height, "VKraft", NULL, NULL);

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	//glfwSetWindowFocusCallback(window, window_focus_callback);
	//glfwSetMouseButtonCallback(window, mouse_button_callback);

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
						.surface(surface.handle())
						.presentMode(VK_PRESENT_MODE_IMMEDIATE_KHR));

	renderPass.create(VKLRenderPassCreateInfo().device(&device)
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
	
	VKLImageCreateInfo backBuffersCreateInfo;
	backBuffersCreateInfo.device(&device)
						.extent(800, 600, 1)
						.format(VK_FORMAT_R16G16B16A16_SFLOAT)
						.usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
						.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY);
	
	backBuffers[0].create(backBuffersCreateInfo);
	
	backBuffersCreateInfo.usage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT);
	
	backBuffers[1].create(backBuffersCreateInfo);
	
	transferQueue->getCmdBuffer()->begin();
	
	backBuffers[0].cmdTransitionBarrier(transferQueue->getCmdBuffer(), VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
									VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
	
	backBuffers[1].cmdTransitionBarrier(transferQueue->getCmdBuffer(), VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
									   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
	
	transferQueue->getCmdBuffer()->end();
	
	transferQueue->submit(transferQueue->getCmdBuffer(), VK_NULL_HANDLE);
	transferQueue->waitIdle();
	
	backBufferViews[0].create(VKLImageViewCreateInfo().image(&backBuffers[0]));
	backBufferViews[1].create(VKLImageViewCreateInfo().image(&backBuffers[1]));
	
	framebuffer.create(VKLFramebufferCreateInfo()
							.renderPass(&renderPass)
							.addAttachment(&backBufferViews[0])
							.addAttachment(&backBufferViews[1])
							.extent(800, 600, 1));
	
	VkClearValue clearColor;
	clearColor.color.float32[0] = 0.25f;
	clearColor.color.float32[1] = 0.45f;
	clearColor.color.float32[2] = 1.0f;
	clearColor.color.float32[3] = 1.0f;
	
	framebuffer.setClearValue(clearColor, 0);
	framebuffer.setClearValue(clearColor, 1);
	
	cmdBuffer = new VKLCommandBuffer(graphicsQueue);
	
	cursor = new Cursor(this);
}

void Application::mainLoop() {
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		
		//if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		//	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		//}
		
		cmdBuffer->begin();
		
		framebuffer.beginRenderPass(cmdBuffer, VK_SUBPASS_CONTENTS_INLINE);
		
		cmdBuffer->nextSubpass(VK_SUBPASS_CONTENTS_INLINE);
		
		cursor->render(cmdBuffer);
		
		cmdBuffer->endRenderPass();
		
		cmdBuffer->end();
		
		graphicsQueue->submit(cmdBuffer, VK_NULL_HANDLE);
		graphicsQueue->waitIdle();

		swapChain.present(&backBuffers[0]);
	}
}

void Application::destroy() {
	cursor->destroy();
	delete cursor;
	
	cmdBuffer->destroy();
	delete cmdBuffer;
	
	framebuffer.destroy();
	
	backBufferViews[0].destroy();
	backBufferViews[1].destroy();
	
	backBuffers[0].destroy();
	backBuffers[1].destroy();
	
	renderPass.destroy();
	swapChain.destroy();
	
	device.destroy();
	surface.destroy();
	instance.destroy();
	
	glfwDestroyWindow(window);
	glfwTerminate();
}
