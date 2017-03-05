#include "VLKUtils.h"
#include <windows.h>
#include <vector>
#include "lodepng.h"

static char* readFile(char* path, uint32_t* size) {
	char *buffer = NULL;
	int read_size;
	FILE *handler = fopen(path, "r");

	fseek(handler, 0, SEEK_END);
	read_size = ftell(handler);
	rewind(handler);
	buffer = (char*)malloc(sizeof(char) * (read_size) + 1);
	fread(buffer, sizeof(char), read_size, handler);

	buffer[read_size] = NULL;

	fclose(handler);

	*size = (uint32_t)read_size;

	return buffer;
}

#ifdef _DEBUG
PFN_vkCreateDebugReportCallbackEXT pfnCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT pfnDestroyDebugReportCallbackEXT;

VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportCallback(VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location,
	int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData) {

	std::cout << pLayerPrefix << " " << pMessage << "\n";
	return VK_FALSE;
}
#endif

void VLKCheck(VkResult result, char *msg) {
	assert(result == VK_SUCCESS, msg);
}

VLKContext* vlkCreateContext() {
	VLKContext* context = (VLKContext*)malloc(sizeof(VLKContext));

#ifdef _DEBUG
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);
	if (layerCount == 0) {
		std::cout << "Could ont find any layers!\n";
		system("PAUSE");
		exit(-1);
	}

	VkLayerProperties* layersAvailable = new VkLayerProperties[layerCount];
	vkEnumerateInstanceLayerProperties(&layerCount, layersAvailable);

	bool foundValidation = false;
	for (int i = 0; i < layerCount; ++i) {
		if (strcmp(layersAvailable[i].layerName, "VK_LAYER_LUNARG_standard_validation") == 0) {
			foundValidation = true;
		}
	}

	if (!foundValidation) {
		std::cout << "Could ont find any layers!\n";
		system("PAUSE");
		exit(-1);
	}

	char* layers[] = { "VK_LAYER_LUNARG_standard_validation" };
#endif

	uint32_t extensionCountGLFW = 0;
	char** extensionsGLFW = (char**)glfwGetRequiredInstanceExtensions(&extensionCountGLFW);

#ifdef _DEBUG
	uint32_t extensionCountEXT = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCountEXT, NULL);
	VkExtensionProperties *extensionsAvailable = new VkExtensionProperties[extensionCountEXT];
	vkEnumerateInstanceExtensionProperties(NULL, &extensionCountEXT, extensionsAvailable);

	char *extentionEXT = "VK_EXT_debug_report";
	uint32_t foundExtensions = 0;
	for (uint32_t i = 0; i < extensionCountEXT; ++i) {
		if (strcmp(extensionsAvailable[i].extensionName, extentionEXT) == 0) {
			foundExtensions++;
		}
	}

	if (!foundExtensions) {
		std::cout << "Could not get required extentions!\n";
		system("PAUSE");
		exit(-1);
	}

	uint32_t extensionCount = extensionCountGLFW + 1;
	char** extensions = new char*[extensionCount];
	for (int i = 0; i < extensionCountGLFW; i++) {
		extensions[i] = extensionsGLFW[i];
	}
	extensions[extensionCountGLFW] = extentionEXT;
#else
	uint32_t extensionCount = extensionCountGLFW;
	char** extensions = extensionsGLFW;
#endif

	VkApplicationInfo applicationInfo = {};
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = NULL;
	applicationInfo.pApplicationName = "VKraft";
	applicationInfo.applicationVersion = 1;
	applicationInfo.pEngineName = "VKraft Engine";
	applicationInfo.engineVersion = 1;
	applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

#ifdef _DEBUG
	context->layers = (char**)malloc(sizeof(char*) * 1);
	context->layers[0] = layers[0];
	context->layerCount = 1;
#else
	context->layers = NULL;
	context->layerCount = 0;
#endif

	VkInstanceCreateInfo instanceInfo = {};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pApplicationInfo = &applicationInfo;
	instanceInfo.enabledLayerCount = context->layerCount;
	instanceInfo.ppEnabledLayerNames = context->layers;
	instanceInfo.enabledExtensionCount = extensionCount;
	instanceInfo.ppEnabledExtensionNames = extensions;

	PFN_vkCreateInstance pfnCreateInstance = (PFN_vkCreateInstance)
		glfwGetInstanceProcAddress(NULL, "vkCreateInstance");

	VLKCheck(pfnCreateInstance(&instanceInfo, NULL, &context->instance),
		"Failed to create Vulkan Instance");

#ifdef _DEBUG
	pfnCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)glfwGetInstanceProcAddress(context->instance, "vkCreateDebugReportCallbackEXT");
	pfnDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)glfwGetInstanceProcAddress(context->instance, "vkDestroyDebugReportCallbackEXT");

	VkDebugReportCallbackCreateInfoEXT callbackCreateInfo = {};
	callbackCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	callbackCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
		VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
	callbackCreateInfo.pfnCallback = &DebugReportCallback;
	callbackCreateInfo.pUserData = NULL;

	VLKCheck(pfnCreateDebugReportCallbackEXT(context->instance, &callbackCreateInfo, NULL, &context->callback),
		"Could not create Debug Callback");
#endif

	return context;
}

void vlkDestroyContext(VLKContext* context) {
#ifdef _DEBUG
	pfnDestroyDebugReportCallbackEXT(context->instance, context->callback, NULL);
#endif
	vkDestroyInstance(context->instance, NULL);

	free(context);
}

void vlkCreateDeviceAndSwapchain(GLFWwindow* window, VLKContext* context, VLKDevice** pdevice, VLKSwapchain** pswapChain) {
	VLKDevice* device = (VLKDevice*)malloc(sizeof(VLKDevice));
	VLKSwapchain* swapChain = (VLKSwapchain*)malloc(sizeof(VLKSwapchain));

	glfwCreateWindowSurface(context->instance, window, NULL, &swapChain->surface);

	uint32_t physicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(context->instance, &physicalDeviceCount, NULL);
	VkPhysicalDevice *physicalDevices = new VkPhysicalDevice[physicalDeviceCount];
	vkEnumeratePhysicalDevices(context->instance, &physicalDeviceCount, physicalDevices);

	for (uint32_t i = 0; i < physicalDeviceCount; ++i) {
		VkPhysicalDeviceProperties deviceProperties = {};
		vkGetPhysicalDeviceProperties(physicalDevices[i], &deviceProperties);

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyCount, NULL);
		VkQueueFamilyProperties *queueFamilyProperties = new VkQueueFamilyProperties[queueFamilyCount];
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i],
			&queueFamilyCount,
			queueFamilyProperties);

		for (uint32_t j = 0; j < queueFamilyCount; ++j) {

			VkBool32 supportsPresent;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], j, swapChain->surface,
				&supportsPresent);

			if (supportsPresent && (queueFamilyProperties[j].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))) {
				device->physicalDevice = physicalDevices[i];
				device->physicalDeviceProperties = deviceProperties;
				device->presentQueueIdx = j;
				break;
			}
		}
		delete[] queueFamilyProperties;

		if (device->physicalDevice) {
			break;
		}
	}
	delete[] physicalDevices;

	assert(device->physicalDevice, "No physical device detected that can render and present!");

	float queuePriorities[] = { 1.0f, 1.0f };

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = device->presentQueueIdx;
	queueCreateInfo.queueCount = 2;
	queueCreateInfo.pQueuePriorities = queuePriorities;

	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceInfo.enabledLayerCount = context->layerCount;
	deviceInfo.ppEnabledLayerNames = context->layers;

	const char *deviceExtensions[] = { "VK_KHR_swapchain" };
	deviceInfo.enabledExtensionCount = 1;
	deviceInfo.ppEnabledExtensionNames = deviceExtensions;

	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device->physicalDevice, &features);

	if (features.shaderClipDistance != VK_TRUE || features.geometryShader != VK_TRUE) {
		std::cout << "Device does not support required features!\n";
		exit(-1);
	}

	deviceInfo.pEnabledFeatures = &features;

	VLKCheck(vkCreateDevice(device->physicalDevice, &deviceInfo, NULL, &device->device),
		"Failed to create logical device");

	uint32_t formatCount = 0;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device->physicalDevice, swapChain->surface,
		&formatCount, NULL);
	VkSurfaceFormatKHR *surfaceFormats = new VkSurfaceFormatKHR[formatCount];
	vkGetPhysicalDeviceSurfaceFormatsKHR(device->physicalDevice, swapChain->surface,
		&formatCount, surfaceFormats);

	VkFormat colorFormat;
	if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED) {
		colorFormat = VK_FORMAT_B8G8R8_UNORM;
	}
	else {
		colorFormat = surfaceFormats[0].format;
	}
	VkColorSpaceKHR colorSpace;
	colorSpace = surfaceFormats[0].colorSpace;
	delete[] surfaceFormats;

	VkSurfaceCapabilitiesKHR surfaceCapabilities = {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physicalDevice, swapChain->surface,
		&surfaceCapabilities);

	uint32_t desiredImageCount = 2;
	if (desiredImageCount < surfaceCapabilities.minImageCount) {
		desiredImageCount = surfaceCapabilities.minImageCount;
	}
	else if (surfaceCapabilities.maxImageCount != 0 &&
		desiredImageCount > surfaceCapabilities.maxImageCount) {
		desiredImageCount = surfaceCapabilities.maxImageCount;
	}

	int wW, wH;
	glfwGetWindowSize(window, &wW, &wH);

	swapChain->width = (uint32_t)wW;
	swapChain->height = (uint32_t)wH;

	VkExtent2D surfaceResolution = surfaceCapabilities.currentExtent;
	if (surfaceResolution.width == -1) {
		surfaceResolution.width = swapChain->width;
		surfaceResolution.height = swapChain->height;
	} else {
		swapChain->width = surfaceResolution.width;
		swapChain->height = surfaceResolution.height;
	}

	VkSurfaceTransformFlagBitsKHR preTransform = surfaceCapabilities.currentTransform;
	if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}

	uint32_t presentModeCount = 0;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice, swapChain->surface,
		&presentModeCount, NULL);
	VkPresentModeKHR *presentModes = new VkPresentModeKHR[presentModeCount];
	vkGetPhysicalDeviceSurfacePresentModesKHR(device->physicalDevice, swapChain->surface,
		&presentModeCount, presentModes);

	VkPresentModeKHR presentationMode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32_t i = 0; i < presentModeCount; ++i) {
		if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			presentationMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
	}
	delete[] presentModes;

	VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
	swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainCreateInfo.surface = swapChain->surface;
	swapChainCreateInfo.minImageCount = desiredImageCount;
	swapChainCreateInfo.imageFormat = colorFormat;
	swapChainCreateInfo.imageColorSpace = colorSpace;
	swapChainCreateInfo.imageExtent = surfaceResolution;
	swapChainCreateInfo.imageArrayLayers = 1;
	swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapChainCreateInfo.preTransform = preTransform;
	swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapChainCreateInfo.presentMode = presentationMode;
	swapChainCreateInfo.clipped = true;

	VLKCheck(vkCreateSwapchainKHR(device->device, &swapChainCreateInfo, NULL, &swapChain->swapChain),
		"Failed to create swapchain");

	vkGetDeviceQueue(device->device, device->presentQueueIdx, 0, &device->presentQueue);

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = device->presentQueueIdx;

	VLKCheck(vkCreateCommandPool(device->device, &commandPoolCreateInfo, NULL, &device->commandPool),
		"Failed to create command pool");

	VkCommandBufferAllocateInfo commandBufferAllocationInfo = {};
	commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocationInfo.commandPool = device->commandPool;
	commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocationInfo.commandBufferCount = 1;

	VLKCheck(vkAllocateCommandBuffers(device->device, &commandBufferAllocationInfo, &device->setupCmdBuffer),
		"Failed to allocate setup command buffer");

	VLKCheck(vkAllocateCommandBuffers(device->device, &commandBufferAllocationInfo, &device->drawCmdBuffer),
		"Failed to allocate draw command buffer");

	vkGetSwapchainImagesKHR(device->device, swapChain->swapChain, &swapChain->imageCount, NULL);
	swapChain->presentImages = new VkImage[swapChain->imageCount];
	vkGetSwapchainImagesKHR(device->device, swapChain->swapChain, &swapChain->imageCount, swapChain->presentImages);

	VkImageViewCreateInfo presentImagesViewCreateInfo = {};
	presentImagesViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	presentImagesViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	presentImagesViewCreateInfo.format = colorFormat;
	presentImagesViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A };
	presentImagesViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	presentImagesViewCreateInfo.subresourceRange.baseMipLevel = 0;
	presentImagesViewCreateInfo.subresourceRange.levelCount = 1;
	presentImagesViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	presentImagesViewCreateInfo.subresourceRange.layerCount = 1;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence submitFence;
	vkCreateFence(device->device, &fenceCreateInfo, NULL, &submitFence);

	bool *transitioned = new bool[swapChain->imageCount];
	memset(transitioned, 0, sizeof(bool) * swapChain->imageCount);
	uint32_t doneCount = 0;
	while (doneCount != swapChain->imageCount) {
		VkSemaphore presentCompleteSemaphore;
		VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 0, 0 };
		vkCreateSemaphore(device->device, &semaphoreCreateInfo, NULL, &presentCompleteSemaphore);

		uint32_t nextImageIdx;
		vkAcquireNextImageKHR(device->device, swapChain->swapChain, UINT64_MAX,
			presentCompleteSemaphore, VK_NULL_HANDLE, &nextImageIdx);

		if (!transitioned[nextImageIdx]) {
			vkBeginCommandBuffer(device->setupCmdBuffer, &beginInfo);

			VkImageMemoryBarrier layoutTransitionBarrier = {};
			layoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			layoutTransitionBarrier.srcAccessMask = 0;
			layoutTransitionBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			layoutTransitionBarrier.image = swapChain->presentImages[nextImageIdx];
			VkImageSubresourceRange resourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			layoutTransitionBarrier.subresourceRange = resourceRange;

			vkCmdPipelineBarrier(device->setupCmdBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				0,
				0, NULL,
				0, NULL,
				1, &layoutTransitionBarrier);

			vkEndCommandBuffer(device->setupCmdBuffer);

			VkPipelineStageFlags waitStageMash[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
			submitInfo.pWaitDstStageMask = waitStageMash;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &device->setupCmdBuffer;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores = NULL;
			VLKCheck(vkQueueSubmit(device->presentQueue, 1, &submitInfo, submitFence),
				"Could not submit queue");

			vkWaitForFences(device->device, 1, &submitFence, VK_TRUE, UINT64_MAX);
			vkResetFences(device->device, 1, &submitFence);

			vkDestroySemaphore(device->device, presentCompleteSemaphore, NULL);

			vkResetCommandBuffer(device->setupCmdBuffer, 0);

			transitioned[nextImageIdx] = true;
			doneCount++;

		}

		VkPresentInfoKHR presentInfo = {};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 0;
		presentInfo.pWaitSemaphores = NULL;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain->swapChain;
		presentInfo.pImageIndices = &nextImageIdx;
		vkQueuePresentKHR(device->presentQueue, &presentInfo);

	}
	delete[] transitioned;

	swapChain->presentImageViews = new VkImageView[swapChain->imageCount];
	for (uint32_t i = 0; i < swapChain->imageCount; ++i) {
		presentImagesViewCreateInfo.image = swapChain->presentImages[i];

		VLKCheck(vkCreateImageView(device->device, &presentImagesViewCreateInfo, NULL, &swapChain->presentImageViews[i]),
			"Coud not create image view");
	}

	vkGetPhysicalDeviceMemoryProperties(device->physicalDevice, &device->memoryProperties);

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_D16_UNORM;
	imageCreateInfo.extent = { swapChain->width, swapChain->height, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = NULL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VLKCheck(vkCreateImage(device->device, &imageCreateInfo, NULL, &swapChain->depthImage),
		"Failed to create depth image");

	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(device->device, swapChain->depthImage, &memoryRequirements);

	VkMemoryAllocateInfo imageAllocateInfo = {};
	imageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imageAllocateInfo.allocationSize = memoryRequirements.size;

	uint32_t memoryTypeBits = memoryRequirements.memoryTypeBits;
	VkMemoryPropertyFlags desiredMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	for (uint32_t i = 0; i < 32; ++i) {
		VkMemoryType memoryType = device->memoryProperties.memoryTypes[i];
		if (memoryTypeBits & 1) {
			if ((memoryType.propertyFlags & desiredMemoryFlags) == desiredMemoryFlags) {
				imageAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
		memoryTypeBits = memoryTypeBits >> 1;
	}

	VLKCheck(vkAllocateMemory(device->device, &imageAllocateInfo, NULL, &swapChain->imageMemory),
		"Failed to allocate device memory");

	VLKCheck(vkBindImageMemory(device->device, swapChain->depthImage, swapChain->imageMemory, 0),
		"Failed to bind image memory");

	vkBeginCommandBuffer(device->setupCmdBuffer, &beginInfo);

	VkImageMemoryBarrier layoutTransitionBarrier = {};
	layoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	layoutTransitionBarrier.srcAccessMask = 0;
	layoutTransitionBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.image = swapChain->depthImage;
	VkImageSubresourceRange resourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
	layoutTransitionBarrier.subresourceRange = resourceRange;

	vkCmdPipelineBarrier(device->setupCmdBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &layoutTransitionBarrier);

	vkEndCommandBuffer(device->setupCmdBuffer);

	VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = waitStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &device->setupCmdBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;
	VLKCheck(vkQueueSubmit(device->presentQueue, 1, &submitInfo, submitFence),
		"Could not submit Queue");

	vkWaitForFences(device->device, 1, &submitFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device->device, 1, &submitFence);
	vkDestroyFence(device->device, submitFence, NULL);
	vkResetCommandBuffer(device->setupCmdBuffer, 0);

	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = swapChain->depthImage;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = imageCreateInfo.format;
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	VLKCheck(vkCreateImageView(device->device, &imageViewCreateInfo, NULL, &swapChain->depthImageView),
		"Failed to create image view");

	VkAttachmentDescription passAttachments[2] = {};
	passAttachments[0].format = colorFormat;
	passAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	passAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	passAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	passAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	passAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	passAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	passAttachments[1].format = VK_FORMAT_D16_UNORM;
	passAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	passAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	passAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	passAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	passAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = passAttachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;

	VLKCheck(vkCreateRenderPass(device->device, &renderPassCreateInfo, NULL, &swapChain->renderPass),
		"Failed to create renderpass");

	VkImageView frameBufferAttachments[2];
	frameBufferAttachments[1] = swapChain->depthImageView;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.renderPass = swapChain->renderPass;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = frameBufferAttachments;
	frameBufferCreateInfo.width = swapChain->width;
	frameBufferCreateInfo.height = swapChain->height;
	frameBufferCreateInfo.layers = 1;

	swapChain->frameBuffers = new VkFramebuffer[swapChain->imageCount];
	for (uint32_t i = 0; i < swapChain->imageCount; ++i) {
		frameBufferAttachments[0] = swapChain->presentImageViews[i];
		VLKCheck(vkCreateFramebuffer(device->device, &frameBufferCreateInfo, NULL, &swapChain->frameBuffers[i]),
			"Failed to create framebuffer");
	}

	*pdevice = device;
	*pswapChain = swapChain;
}

void vlkDestroyDeviceAndSwapchain(VLKContext* context, VLKDevice* device, VLKSwapchain* swapChain) {
	vkFreeMemory(device->device, swapChain->imageMemory, NULL);

	for (uint32_t i = 0; i < swapChain->imageCount; ++i) {
		vkDestroyFramebuffer(device->device, swapChain->frameBuffers[i], NULL);
		vkDestroyImageView(device->device, swapChain->presentImageViews[i], NULL);
	}

	vkDestroyImageView(device->device, swapChain->depthImageView, NULL);
	vkDestroyImage(device->device, swapChain->depthImage, NULL);

	vkDestroyRenderPass(device->device, swapChain->renderPass, NULL);

	vkDestroySwapchainKHR(device->device, swapChain->swapChain, NULL);

	vkDestroyCommandPool(device->device, device->commandPool, NULL);
	vkDestroyDevice(device->device, NULL);
	vkDestroySurfaceKHR(context->instance, swapChain->surface, NULL);

	free(swapChain);
	free(device);
}

VLKModel* vlkCreateModel(VLKDevice* device, Vertex* verts, uint32_t num) {
	VLKModel* model = (VLKModel*)malloc(sizeof(VLKModel));

	VkBufferCreateInfo vertexInputBufferInfo = {};
	vertexInputBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexInputBufferInfo.size = sizeof(Vertex) * num;
	vertexInputBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	vertexInputBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VLKCheck(vkCreateBuffer(device->device, &vertexInputBufferInfo, NULL, &model->vertexInputBuffer),
		"Failed to create vertex input buffer.");

	VkMemoryRequirements vertexBufferMemoryRequirements = {};
	vkGetBufferMemoryRequirements(device->device, model->vertexInputBuffer,
		&vertexBufferMemoryRequirements);

	VkMemoryAllocateInfo bufferAllocateInfo = {};
	bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	bufferAllocateInfo.allocationSize = vertexBufferMemoryRequirements.size;

	uint32_t vertexMemoryTypeBits = vertexBufferMemoryRequirements.memoryTypeBits;
	VkMemoryPropertyFlags vertexDesiredMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	for (uint32_t i = 0; i < 32; ++i) {
		VkMemoryType memoryType = device->memoryProperties.memoryTypes[i];
		if (vertexMemoryTypeBits & 1) {
			if ((memoryType.propertyFlags & vertexDesiredMemoryFlags) == vertexDesiredMemoryFlags) {
				bufferAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
		vertexMemoryTypeBits = vertexMemoryTypeBits >> 1;
	}

	VLKCheck(vkAllocateMemory(device->device, &bufferAllocateInfo, NULL, &model->vertexBufferMemory),
		"Failed to allocate buffer memory");

	void *mapped;
	VLKCheck(vkMapMemory(device->device, model->vertexBufferMemory, 0, VK_WHOLE_SIZE, 0, &mapped),
		"Failed to map buffer memory");

	memcpy(mapped, verts, sizeof(Vertex) * num);

	vkUnmapMemory(device->device, model->vertexBufferMemory);

	VLKCheck(vkBindBufferMemory(device->device, model->vertexInputBuffer, model->vertexBufferMemory, 0),
		"Failed to bind buffer memory");

	return model;
}

void vlkDestroyModel(VLKDevice* device, VLKModel* model) {
	vkFreeMemory(device->device, model->vertexBufferMemory, NULL);
	vkDestroyBuffer(device->device, model->vertexInputBuffer, NULL);
	free(model);
}

VLKShader* vlkCreateShader(VLKDevice* device, char* vertPath, char* fragPath, void* uniformBuffer, uint32_t uniformSize) {
	VLKShader* shader = (VLKShader*)malloc(sizeof(VLKShader));

	uint32_t codeSize;
	char* code = new char[20000];
	HANDLE fileHandle = 0;

	fileHandle = CreateFile(vertPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		OutputDebugStringA("Failed to open shader file.");
		exit(1);
	}
	ReadFile((HANDLE)fileHandle, code, 20000, (LPDWORD)&codeSize, 0);
	CloseHandle(fileHandle);

	VkShaderModuleCreateInfo vertexShaderCreationInfo = {};
	vertexShaderCreationInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertexShaderCreationInfo.codeSize = codeSize;
	vertexShaderCreationInfo.pCode = (uint32_t *)code;

	VLKCheck(vkCreateShaderModule(device->device, &vertexShaderCreationInfo, NULL, &shader->vertexShaderModule),
		"Failed to create vertex shader module");

	fileHandle = CreateFile(fragPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) {
		OutputDebugStringA("Failed to open shader file.");
		exit(1);
	}
	ReadFile((HANDLE)fileHandle, code, 20000, (LPDWORD)&codeSize, 0);
	CloseHandle(fileHandle);

	VkShaderModuleCreateInfo fragmentShaderCreationInfo = {};
	fragmentShaderCreationInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fragmentShaderCreationInfo.codeSize = codeSize;
	fragmentShaderCreationInfo.pCode = (uint32_t *)code;

	VLKCheck(vkCreateShaderModule(device->device, &fragmentShaderCreationInfo, NULL, &shader->fragmentShaderModule),
		"Could not create Fragment shader");

	delete[] code;

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = uniformSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VLKCheck(vkCreateBuffer(device->device, &bufferCreateInfo, NULL, &shader->buffer),
		"Failed to create uniforms buffer");

	VkMemoryRequirements bufferMemoryRequirements = {};
	vkGetBufferMemoryRequirements(device->device, shader->buffer, &bufferMemoryRequirements);

	VkMemoryAllocateInfo matrixAllocateInfo = {};
	matrixAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	matrixAllocateInfo.allocationSize = bufferMemoryRequirements.size;

	uint32_t uniformMemoryTypeBits = bufferMemoryRequirements.memoryTypeBits;
	VkMemoryPropertyFlags uniformDesiredMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	for (uint32_t i = 0; i < 32; ++i) {
		VkMemoryType memoryType = device->memoryProperties.memoryTypes[i];
		if (uniformMemoryTypeBits & 1) {
			if ((memoryType.propertyFlags & uniformDesiredMemoryFlags) == uniformDesiredMemoryFlags) {
				matrixAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
		uniformMemoryTypeBits = uniformMemoryTypeBits >> 1;
	}

	VLKCheck(vkAllocateMemory(device->device, &matrixAllocateInfo, NULL, &shader->memory),
		"Failed to allocate uniforms buffer memory");

	VLKCheck(vkBindBufferMemory(device->device, shader->buffer, shader->memory, 0),
		"Failed to bind uniforms buffer memory");

	void *matrixMapped;
	VLKCheck(vkMapMemory( device->device, shader->memory, 0, VK_WHOLE_SIZE, 0, &matrixMapped ),
		"Failed to map uniform buffer memory." );

	memcpy( matrixMapped, uniformBuffer, uniformSize);

	vkUnmapMemory(device->device, shader->memory);

	VkDescriptorSetLayoutBinding bindings[2];
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	bindings[0].pImmutableSamplers = NULL;

	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[1].pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {};
	setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setLayoutCreateInfo.bindingCount = 2;
	setLayoutCreateInfo.pBindings = bindings;

	VLKCheck(vkCreateDescriptorSetLayout(device->device, &setLayoutCreateInfo, NULL, &shader->setLayout),
		"Failed to create DescriptorSetLayout");

	VkDescriptorPoolSize uniformBufferPoolSize[2];
	uniformBufferPoolSize[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferPoolSize[0].descriptorCount = 1;
	uniformBufferPoolSize[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uniformBufferPoolSize[1].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = 1;
	poolCreateInfo.poolSizeCount = 2;
	poolCreateInfo.pPoolSizes = uniformBufferPoolSize;

	VLKCheck(vkCreateDescriptorPool(device->device, &poolCreateInfo, NULL, &shader->descriptorPool),
		"Failed to create descriptor pool");

	VkDescriptorSetAllocateInfo descriptorAllocateInfo = {};
	descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorAllocateInfo.descriptorPool = shader->descriptorPool;
	descriptorAllocateInfo.descriptorSetCount = 1;
	descriptorAllocateInfo.pSetLayouts = &shader->setLayout;

	VLKCheck(vkAllocateDescriptorSets(device->device, &descriptorAllocateInfo, &shader->descriptorSet),
		"Failed to allocate descriptor sets");

	VkDescriptorBufferInfo descriptorBufferInfo = {};
	descriptorBufferInfo.buffer = shader->buffer;
	descriptorBufferInfo.offset = 0;
	descriptorBufferInfo.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet writeDescriptor = {};
	writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptor.dstSet = shader->descriptorSet;
	writeDescriptor.dstBinding = 0;
	writeDescriptor.dstArrayElement = 0;
	writeDescriptor.descriptorCount = 1;
	writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptor.pImageInfo = NULL;
	writeDescriptor.pBufferInfo = &descriptorBufferInfo;
	writeDescriptor.pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device->device, 1, &writeDescriptor, 0, NULL);

	return shader;
}

void vlkUniforms(VLKDevice* device, VLKShader* shader, void* uniformBuffer, uint32_t uniformSize) {
	void *matrixMapped;
	vkMapMemory(device->device, shader->memory, 0, VK_WHOLE_SIZE, 0, &matrixMapped);

	memcpy(matrixMapped, uniformBuffer, uniformSize);

	VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.memory = shader->memory;
	memoryRange.offset = 0;
	memoryRange.size = VK_WHOLE_SIZE;
	vkFlushMappedMemoryRanges(device->device, 1, &memoryRange);

	vkUnmapMemory(device->device, shader->memory);
}

void vlkDestroyShader(VLKDevice* device, VLKShader* shader) {
	vkFreeMemory(device->device, shader->memory, NULL);

	vkDestroyDescriptorPool(device->device, shader->descriptorPool, NULL);
	vkDestroyDescriptorSetLayout(device->device, shader->setLayout, NULL);

	vkDestroyBuffer(device->device, shader->buffer, NULL);

	vkDestroyShaderModule(device->device, shader->vertexShaderModule, NULL);
	vkDestroyShaderModule(device->device, shader->fragmentShaderModule, NULL);

	free(shader);
}

VLKPipeline* vlkCreatePipeline(VLKDevice* device, VLKSwapchain* swapChain, VLKShader* shader) {
	VLKPipeline* pipeline = (VLKPipeline*)malloc(sizeof(VLKPipeline));

	VkPipelineLayoutCreateInfo layoutCreateInfo = {};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &shader->setLayout;
	layoutCreateInfo.pushConstantRangeCount = 0;
	layoutCreateInfo.pPushConstantRanges = NULL;

	VLKCheck(vkCreatePipelineLayout(device->device, &layoutCreateInfo, NULL, &pipeline->pipelineLayout),
		"Failed to create pipeline layout");

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo[2] = {};
	shaderStageCreateInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStageCreateInfo[0].module = shader->vertexShaderModule;
	shaderStageCreateInfo[0].pName = "main";
	shaderStageCreateInfo[0].pSpecializationInfo = NULL;

	shaderStageCreateInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStageCreateInfo[1].module = shader->fragmentShaderModule;
	shaderStageCreateInfo[1].pName = "main";
	shaderStageCreateInfo[1].pSpecializationInfo = NULL;

	VkVertexInputBindingDescription vertexBindingDescription = {};
	vertexBindingDescription.binding = 0;
	vertexBindingDescription.stride = sizeof(Vertex);
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertexAttributeDescritpion[2];
	vertexAttributeDescritpion[0].location = 0;
	vertexAttributeDescritpion[0].binding = 0;
	vertexAttributeDescritpion[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	vertexAttributeDescritpion[0].offset = 0;

	vertexAttributeDescritpion[1].location = 1;
	vertexAttributeDescritpion[1].binding = 0;
	vertexAttributeDescritpion[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescritpion[1].offset = 4 * sizeof(float);

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 2;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = vertexAttributeDescritpion;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {};
	inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = swapChain->width;
	viewport.height = swapChain->height;
	viewport.minDepth = 0;
	viewport.maxDepth = 1;

	VkRect2D scissors = {};
	scissors.offset = { 0, 0 };
	scissors.extent = { swapChain->width, swapChain->height };

	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissors;

	VkPipelineRasterizationStateCreateInfo rasterizationState = {};
	rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.depthBiasConstantFactor = 0;
	rasterizationState.depthBiasClamp = 0;
	rasterizationState.depthBiasSlopeFactor = 0;
	rasterizationState.lineWidth = 1;

	VkPipelineMultisampleStateCreateInfo multisampleState = {};
	multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampleState.sampleShadingEnable = VK_FALSE;
	multisampleState.minSampleShading = 0;
	multisampleState.pSampleMask = NULL;
	multisampleState.alphaToCoverageEnable = VK_FALSE;
	multisampleState.alphaToOneEnable = VK_FALSE;

	VkStencilOpState noOPStencilState = {};
	noOPStencilState.failOp = VK_STENCIL_OP_KEEP;
	noOPStencilState.passOp = VK_STENCIL_OP_KEEP;
	noOPStencilState.depthFailOp = VK_STENCIL_OP_KEEP;
	noOPStencilState.compareOp = VK_COMPARE_OP_ALWAYS;
	noOPStencilState.compareMask = 0;
	noOPStencilState.writeMask = 0;
	noOPStencilState.reference = 0;

	VkPipelineDepthStencilStateCreateInfo depthState = {};
	depthState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthState.depthTestEnable = VK_TRUE;
	depthState.depthWriteEnable = VK_TRUE;
	depthState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthState.depthBoundsTestEnable = VK_FALSE;
	depthState.stencilTestEnable = VK_FALSE;
	depthState.front = noOPStencilState;
	depthState.back = noOPStencilState;
	depthState.minDepthBounds = 0;
	depthState.maxDepthBounds = 0;

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.blendEnable = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentState.colorWriteMask = 0xf;

	VkPipelineColorBlendStateCreateInfo colorBlendState = {};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_CLEAR;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &colorBlendAttachmentState;
	colorBlendState.blendConstants[0] = 0.0;
	colorBlendState.blendConstants[1] = 0.0;
	colorBlendState.blendConstants[2] = 0.0;
	colorBlendState.blendConstants[3] = 0.0;

	VkDynamicState dynamicState[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = 2;
	dynamicStateCreateInfo.pDynamicStates = dynamicState;

	VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStageCreateInfo;
	pipelineCreateInfo.pVertexInputState = &vertexInputStateCreateInfo;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
	pipelineCreateInfo.pTessellationState = NULL;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pDepthStencilState = &depthState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	pipelineCreateInfo.layout = pipeline->pipelineLayout;
	pipelineCreateInfo.renderPass = swapChain->renderPass;
	pipelineCreateInfo.subpass = 0;
	pipelineCreateInfo.basePipelineHandle = NULL;
	pipelineCreateInfo.basePipelineIndex = 0;

	VLKCheck(vkCreateGraphicsPipelines(device->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &pipeline->pipeline),
		"Failed to create graphics pipeline");

	return pipeline;
}

void vlkDestroyPipeline(VLKDevice* device, VLKPipeline* pipeline) {
	vkDestroyPipeline(device->device, pipeline->pipeline, NULL);
	vkDestroyPipelineLayout(device->device, pipeline->pipelineLayout, NULL);

	free(pipeline);
}

void vlkClear(VLKDevice* device, VLKSwapchain* swapChain) {
	VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 0, 0 };
	vkCreateSemaphore(device->device, &semaphoreCreateInfo, NULL, &swapChain->presentCompleteSemaphore);
	vkCreateSemaphore(device->device, &semaphoreCreateInfo, NULL, &swapChain->renderingCompleteSemaphore);

	vkAcquireNextImageKHR(device->device, swapChain->swapChain, UINT64_MAX,
		swapChain->presentCompleteSemaphore, VK_NULL_HANDLE, &swapChain->nextImageIdx);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(device->drawCmdBuffer, &beginInfo);

	VkImageMemoryBarrier layoutTransitionBarrier = {};
	layoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	layoutTransitionBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	layoutTransitionBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.image = swapChain->presentImages[swapChain->nextImageIdx];
	VkImageSubresourceRange resourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	layoutTransitionBarrier.subresourceRange = resourceRange;

	vkCmdPipelineBarrier(device->drawCmdBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &layoutTransitionBarrier);

	VkClearValue clearValue[] = { { 0.25f, 0.45f, 1.0f, 1.0f },{ 1.0, 0.0 } };
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = swapChain->renderPass;
	renderPassBeginInfo.framebuffer = swapChain->frameBuffers[swapChain->nextImageIdx];
	renderPassBeginInfo.renderArea = { 0, 0, swapChain->width, swapChain->height };
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValue;
	vkCmdBeginRenderPass(device->drawCmdBuffer, &renderPassBeginInfo,
		VK_SUBPASS_CONTENTS_INLINE);
}

void vlkSwap(VLKDevice* device, VLKSwapchain* swapChain) {
	vkCmdEndRenderPass(device->drawCmdBuffer);

	VkImageMemoryBarrier prePresentBarrier = {};
	prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	prePresentBarrier.image = swapChain->presentImages[swapChain->nextImageIdx];

	vkCmdPipelineBarrier(device->drawCmdBuffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &prePresentBarrier);

	vkEndCommandBuffer(device->drawCmdBuffer);

	VkFence renderFence;
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkCreateFence(device->device, &fenceCreateInfo, NULL, &renderFence);

	VkPipelineStageFlags waitStageMash = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &swapChain->presentCompleteSemaphore;
	submitInfo.pWaitDstStageMask = &waitStageMash;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &device->drawCmdBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &swapChain->renderingCompleteSemaphore;
	vkQueueSubmit(device->presentQueue, 1, &submitInfo, renderFence);

	vkWaitForFences(device->device, 1, &renderFence, VK_TRUE, UINT64_MAX);
	vkDestroyFence(device->device, renderFence, NULL);

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &swapChain->renderingCompleteSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &swapChain->swapChain;
	presentInfo.pImageIndices = &swapChain->nextImageIdx;
	presentInfo.pResults = NULL;
	vkQueuePresentKHR(device->presentQueue, &presentInfo);

	vkDestroySemaphore(device->device, swapChain->presentCompleteSemaphore, NULL);
	vkDestroySemaphore(device->device, swapChain->renderingCompleteSemaphore, NULL);
}

VLKTexture* vlkCreateTexture(VLKDevice* device, char* path) {
	VLKTexture* texture = (VLKTexture*)malloc(sizeof(VLKTexture));

	uint32_t width, height;

	std::vector<unsigned char> imageData;

	unsigned int error = lodepng::decode(imageData, width, height, path);

	texture->width = width;
	texture->height = height;
	texture->data = imageData.data();

	VkImageCreateInfo textureCreateInfo = {};
	textureCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	textureCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	textureCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	textureCreateInfo.extent = { texture->width, texture->height, 1 };
	textureCreateInfo.mipLevels = 1;
	textureCreateInfo.arrayLayers = 1;
	textureCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	textureCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	textureCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	textureCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	textureCreateInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

	VLKCheck(vkCreateImage(device->device, &textureCreateInfo, NULL, &texture->textureImage),
		"Failed to create texture image");

	VkMemoryRequirements textureMemoryRequirements = {};
	vkGetImageMemoryRequirements(device->device, texture->textureImage, &textureMemoryRequirements);

	VkMemoryAllocateInfo textureImageAllocateInfo = {};
	textureImageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	textureImageAllocateInfo.allocationSize = textureMemoryRequirements.size;

	uint32_t textureMemoryTypeBits = textureMemoryRequirements.memoryTypeBits;
	VkMemoryPropertyFlags tDesiredMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
	for (uint32_t i = 0; i < 32; ++i) {
		VkMemoryType memoryType = device->memoryProperties.memoryTypes[i];
		if (textureMemoryTypeBits & 1) {
			if ((memoryType.propertyFlags & tDesiredMemoryFlags) == tDesiredMemoryFlags) {
				textureImageAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
		textureMemoryTypeBits = textureMemoryTypeBits >> 1;
	}

	
	VLKCheck(vkAllocateMemory(device->device, &textureImageAllocateInfo, NULL, &texture->textureImageMemory),
		"Failed to allocate device memory");

	VLKCheck(vkBindImageMemory(device->device, texture->textureImage, texture->textureImageMemory, 0),
		"Failed to bind image memory");
	
	void *imageMapped;
	VLKCheck(vkMapMemory(device->device, texture->textureImageMemory, 0, VK_WHOLE_SIZE, 0, &imageMapped),
		"Failed to map image memory.");

	memcpy(imageMapped, texture->data, texture->width * texture->height * 4);

	VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.memory = texture->textureImageMemory;
	memoryRange.offset = 0;
	memoryRange.size = VK_WHOLE_SIZE;
	vkFlushMappedMemoryRanges(device->device, 1, &memoryRange);

	vkUnmapMemory(device->device, texture->textureImageMemory);

	imageData.clear();

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(device->setupCmdBuffer, &beginInfo);

	VkImageMemoryBarrier layoutTransitionBarrier = {};
	layoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	layoutTransitionBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
	layoutTransitionBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.image = texture->textureImage;
	VkImageSubresourceRange resourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	layoutTransitionBarrier.subresourceRange = resourceRange;

	vkCmdPipelineBarrier(device->setupCmdBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &layoutTransitionBarrier);

	vkEndCommandBuffer(device->setupCmdBuffer);

	VkPipelineStageFlags waitStageMash[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = waitStageMash;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &device->setupCmdBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;
	
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence submitFence;
	vkCreateFence(device->device, &fenceCreateInfo, NULL, &submitFence);

	VLKCheck(vkQueueSubmit(device->presentQueue, 1, &submitInfo, submitFence),
		"Could not submit Queue");

	vkWaitForFences(device->device, 1, &submitFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device->device, 1, &submitFence);
	vkDestroyFence(device->device, submitFence, NULL);
	vkResetCommandBuffer(device->setupCmdBuffer, 0);

	VkImageViewCreateInfo textureImageViewCreateInfo = {};
	textureImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	textureImageViewCreateInfo.image = texture->textureImage;
	textureImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	textureImageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	textureImageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A };
	textureImageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	textureImageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	textureImageViewCreateInfo.subresourceRange.levelCount = 1;
	textureImageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	textureImageViewCreateInfo.subresourceRange.layerCount = 1;

	VLKCheck(vkCreateImageView(device->device, &textureImageViewCreateInfo, NULL, &texture->textureView),
		"Failed to create image view");

	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.mipLodBias = 0;
	samplerCreateInfo.anisotropyEnable = VK_FALSE;
	samplerCreateInfo.minLod = 0;
	samplerCreateInfo.maxLod = 5;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	VLKCheck(vkCreateSampler(device->device, &samplerCreateInfo, NULL, &texture->sampler),
		"Failed to create sampler");

	VkDescriptorImageInfo descriptorImageInfo = {};
	descriptorImageInfo.sampler = texture->sampler;
	descriptorImageInfo.imageView = texture->textureView;
	descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	texture->descriptorImageInfo = descriptorImageInfo;

	return texture;
}

void vlkBindTexture(VLKDevice* device, VLKShader* shader, VLKTexture* texture) {
	VkWriteDescriptorSet writeDescriptor = {};
	writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptor.dstSet = shader->descriptorSet;
	writeDescriptor.dstBinding = 1;
	writeDescriptor.dstArrayElement = 0;
	writeDescriptor.descriptorCount = 1;
	writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptor.pImageInfo = &texture->descriptorImageInfo;
	writeDescriptor.pBufferInfo = NULL;
	writeDescriptor.pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device->device, 1, &writeDescriptor, 0, NULL);
}

void vlkDestroyTexture(VLKDevice* device, VLKTexture* texture) {
	vkFreeMemory(device->device, texture->textureImageMemory, NULL);

	vkDestroySampler(device->device, texture->sampler, NULL);
	vkDestroyImageView(device->device, texture->textureView, NULL);
	vkDestroyImage(device->device, texture->textureImage, NULL);

	free(texture);
}

VLKFramebuffer* vlkCreateFramebuffer(VLKDevice* device, uint32_t imageCount, uint32_t width, uint32_t height) {
	VLKFramebuffer* frameBuffer = (VLKFramebuffer*)malloc(sizeof(VLKFramebuffer));

	frameBuffer->imageCount = imageCount;
	frameBuffer->width = width;
	frameBuffer->height = height;

	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.extent = { frameBuffer->width, frameBuffer->height, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = NULL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VLKCheck(vkCreateImage(device->device, &imageCreateInfo, NULL, &frameBuffer->colorImage),
		"Failed to create depth image");

	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(device->device, frameBuffer->colorImage, &memoryRequirements);

	VkMemoryAllocateInfo imageAllocateInfo = {};
	imageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imageAllocateInfo.allocationSize = memoryRequirements.size;

	uint32_t memoryTypeBits = memoryRequirements.memoryTypeBits;
	VkMemoryPropertyFlags desiredMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	for (uint32_t i = 0; i < 32; ++i) {
		VkMemoryType memoryType = device->memoryProperties.memoryTypes[i];
		if (memoryTypeBits & 1) {
			if ((memoryType.propertyFlags & desiredMemoryFlags) == desiredMemoryFlags) {
				imageAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
		memoryTypeBits = memoryTypeBits >> 1;
	}

	VLKCheck(vkAllocateMemory(device->device, &imageAllocateInfo, NULL, &frameBuffer->colorImageMemory),
		"Failed to allocate device memory");

	VLKCheck(vkBindImageMemory(device->device, frameBuffer->colorImage, frameBuffer->colorImageMemory, 0),
		"Failed to bind image memory");

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	VkFence submitFence;
	vkCreateFence(device->device, &fenceCreateInfo, NULL, &submitFence);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(device->setupCmdBuffer, &beginInfo);

	VkImageMemoryBarrier layoutTransitionBarrier = {};
	layoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	layoutTransitionBarrier.srcAccessMask = 0;
	layoutTransitionBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.image = frameBuffer->colorImage;
	VkImageSubresourceRange resourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	layoutTransitionBarrier.subresourceRange = resourceRange;

	vkCmdPipelineBarrier(device->setupCmdBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &layoutTransitionBarrier);

	vkEndCommandBuffer(device->setupCmdBuffer);

	VkPipelineStageFlags waitStageMash[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = waitStageMash;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &device->setupCmdBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;
	VLKCheck(vkQueueSubmit(device->presentQueue, 1, &submitInfo, submitFence),
		"Could not submit queue");

	vkWaitForFences(device->device, 1, &submitFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device->device, 1, &submitFence);

	vkResetCommandBuffer(device->setupCmdBuffer, 0);
	
	VkImageViewCreateInfo presentImagesViewCreateInfo = {};
	presentImagesViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	presentImagesViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	presentImagesViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	presentImagesViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R,
		VK_COMPONENT_SWIZZLE_G,
		VK_COMPONENT_SWIZZLE_B,
		VK_COMPONENT_SWIZZLE_A };
	presentImagesViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	presentImagesViewCreateInfo.subresourceRange.baseMipLevel = 0;
	presentImagesViewCreateInfo.subresourceRange.levelCount = 1;
	presentImagesViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	presentImagesViewCreateInfo.subresourceRange.layerCount = 1;
	presentImagesViewCreateInfo.image = frameBuffer->colorImage;

	VLKCheck(vkCreateImageView(device->device, &presentImagesViewCreateInfo, NULL, &frameBuffer->colorImageView),
		"Coud not create image view");

	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_D16_UNORM;
	imageCreateInfo.extent = { frameBuffer->width, frameBuffer->height, 1 };
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.queueFamilyIndexCount = 0;
	imageCreateInfo.pQueueFamilyIndices = NULL;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	VLKCheck(vkCreateImage(device->device, &imageCreateInfo, NULL, &frameBuffer->depthImage),
		"Failed to create depth image");

	vkGetImageMemoryRequirements(device->device, frameBuffer->depthImage, &memoryRequirements);

	imageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	imageAllocateInfo.allocationSize = memoryRequirements.size;

	memoryTypeBits = memoryRequirements.memoryTypeBits;
	desiredMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	for (uint32_t i = 0; i < 32; ++i) {
		VkMemoryType memoryType = device->memoryProperties.memoryTypes[i];
		if (memoryTypeBits & 1) {
			if ((memoryType.propertyFlags & desiredMemoryFlags) == desiredMemoryFlags) {
				imageAllocateInfo.memoryTypeIndex = i;
				break;
			}
		}
		memoryTypeBits = memoryTypeBits >> 1;
	}

	VLKCheck(vkAllocateMemory(device->device, &imageAllocateInfo, NULL, &frameBuffer->depthImageMemory),
		"Failed to allocate device memory");

	VLKCheck(vkBindImageMemory(device->device, frameBuffer->depthImage, frameBuffer->depthImageMemory, 0),
		"Failed to bind image memory");

	vkBeginCommandBuffer(device->setupCmdBuffer, &beginInfo);

	layoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	layoutTransitionBarrier.srcAccessMask = 0;
	layoutTransitionBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
		VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.image = frameBuffer->depthImage;
	resourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 };
	layoutTransitionBarrier.subresourceRange = resourceRange;

	vkCmdPipelineBarrier(device->setupCmdBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &layoutTransitionBarrier);

	vkEndCommandBuffer(device->setupCmdBuffer);

	VkPipelineStageFlags waitStageMask[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = waitStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &device->setupCmdBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;
	VLKCheck(vkQueueSubmit(device->presentQueue, 1, &submitInfo, submitFence),
		"Could not submit Queue");

	vkWaitForFences(device->device, 1, &submitFence, VK_TRUE, UINT64_MAX);
	vkResetFences(device->device, 1, &submitFence);
	vkDestroyFence(device->device, submitFence, NULL);
	vkResetCommandBuffer(device->setupCmdBuffer, 0);

	VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.image = frameBuffer->depthImage;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = imageCreateInfo.format;
	imageViewCreateInfo.components = { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
	imageViewCreateInfo.subresourceRange.aspectMask = aspectMask;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;

	VLKCheck(vkCreateImageView(device->device, &imageViewCreateInfo, NULL, &frameBuffer->depthImageView),
		"Failed to create image view");

	VkAttachmentDescription passAttachments[2] = {};
	passAttachments[0].format = VK_FORMAT_R8G8B8A8_UNORM;
	passAttachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	passAttachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	passAttachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	passAttachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	passAttachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	passAttachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	passAttachments[1].format = VK_FORMAT_D16_UNORM;
	passAttachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	passAttachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	passAttachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	passAttachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	passAttachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	passAttachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentReference = {};
	colorAttachmentReference.attachment = 0;
	colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentReference = {};
	depthAttachmentReference.attachment = 1;
	depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentReference;
	subpass.pDepthStencilAttachment = &depthAttachmentReference;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 2;
	renderPassCreateInfo.pAttachments = passAttachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;

	VLKCheck(vkCreateRenderPass(device->device, &renderPassCreateInfo, NULL, &frameBuffer->renderPass),
		"Failed to create renderpass");

	VkImageView frameBufferAttachments[2];
	frameBufferAttachments[0] = frameBuffer->colorImageView;
	frameBufferAttachments[1] = frameBuffer->depthImageView;

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.renderPass = frameBuffer->renderPass;
	frameBufferCreateInfo.attachmentCount = 2;
	frameBufferCreateInfo.pAttachments = frameBufferAttachments;
	frameBufferCreateInfo.width = frameBuffer->width;
	frameBufferCreateInfo.height = frameBuffer->height;
	frameBufferCreateInfo.layers = 1;

	VLKCheck(vkCreateFramebuffer(device->device, &frameBufferCreateInfo, NULL, &frameBuffer->frameBuffer),
		"Failed to create framebuffer");

	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.mipLodBias = 0;
	samplerCreateInfo.anisotropyEnable = VK_FALSE;
	samplerCreateInfo.minLod = 0;
	samplerCreateInfo.maxLod = 5;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	VLKCheck(vkCreateSampler(device->device, &samplerCreateInfo, NULL, &frameBuffer->sampler),
		"Failed to create sampler");

	VkDescriptorImageInfo descriptorImageInfo = {};
	descriptorImageInfo.sampler = frameBuffer->sampler;
	descriptorImageInfo.imageView = frameBuffer->colorImageView;
	descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	frameBuffer->descriptorImageInfo = descriptorImageInfo;

	VkCommandBufferAllocateInfo commandBufferAllocationInfo = {};
	commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocationInfo.commandPool = device->commandPool;
	commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocationInfo.commandBufferCount = 1;

	VLKCheck(vkAllocateCommandBuffers(device->device, &commandBufferAllocationInfo, &frameBuffer->drawCmdBuffer),
		"Failed to allocate setup command buffer");
	
	return frameBuffer;
}

void vlkDestroyFramebuffer(VLKDevice* device, VLKFramebuffer* frameBuffer) {
	vkDestroySampler(device->device, frameBuffer->sampler, NULL);
	vkFreeCommandBuffers(device->device, device->commandPool, 1, &frameBuffer->drawCmdBuffer);

	vkDestroyFramebuffer(device->device, frameBuffer->frameBuffer, NULL);

	vkDestroyRenderPass(device->device, frameBuffer->renderPass, NULL);

	vkFreeMemory(device->device, frameBuffer->depthImageMemory, NULL);
	vkDestroyImageView(device->device, frameBuffer->depthImageView, NULL);
	vkDestroyImage(device->device, frameBuffer->depthImage, NULL);

	vkFreeMemory(device->device, frameBuffer->colorImageMemory, NULL);
	vkDestroyImageView(device->device, frameBuffer->colorImageView, NULL);
	vkDestroyImage(device->device, frameBuffer->colorImage, NULL);
}

void vlkStartFramebuffer(VLKDevice* device, VLKFramebuffer* frameBuffer) {
	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(frameBuffer->drawCmdBuffer, &beginInfo);

	VkImageMemoryBarrier layoutTransitionBarrier = {};
	layoutTransitionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	layoutTransitionBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	layoutTransitionBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
		VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	layoutTransitionBarrier.image = frameBuffer->colorImage;
	VkImageSubresourceRange resourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	layoutTransitionBarrier.subresourceRange = resourceRange;

	vkCmdPipelineBarrier(frameBuffer->drawCmdBuffer,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &layoutTransitionBarrier);

	VkClearValue clearValue[] = { { 0.25f, 0.45f, 1.0f, 1.0f },{ 1.0, 0.0 } };
	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = frameBuffer->renderPass;
	renderPassBeginInfo.framebuffer = frameBuffer->frameBuffer;
	renderPassBeginInfo.renderArea = { 0, 0, frameBuffer->width, frameBuffer->height };
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValue;
	vkCmdBeginRenderPass(frameBuffer->drawCmdBuffer, &renderPassBeginInfo,
		VK_SUBPASS_CONTENTS_INLINE);
}

void vlkEndFramebuffer(VLKDevice* device, VLKFramebuffer* frameBuffer) {
	vkCmdEndRenderPass(frameBuffer->drawCmdBuffer);

	VkImageMemoryBarrier prePresentBarrier = {};
	prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	prePresentBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	prePresentBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	prePresentBarrier.image = frameBuffer->colorImage;

	vkCmdPipelineBarrier(frameBuffer->drawCmdBuffer,
		VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0,
		0, NULL,
		0, NULL,
		1, &prePresentBarrier);

	vkEndCommandBuffer(frameBuffer->drawCmdBuffer);

	VkFence renderFence;
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	vkCreateFence(device->device, &fenceCreateInfo, NULL, &renderFence);

	VkPipelineStageFlags waitStageMash = { VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = 0;
	submitInfo.pWaitSemaphores = NULL;
	submitInfo.pWaitDstStageMask = &waitStageMash;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &frameBuffer->drawCmdBuffer;
	submitInfo.signalSemaphoreCount = 0;
	submitInfo.pSignalSemaphores = NULL;
	vkQueueSubmit(device->presentQueue, 1, &submitInfo, renderFence);

	vkWaitForFences(device->device, 1, &renderFence, VK_TRUE, UINT64_MAX);
	vkDestroyFence(device->device, renderFence, NULL);

	vkResetCommandBuffer(frameBuffer->drawCmdBuffer, 0);
}