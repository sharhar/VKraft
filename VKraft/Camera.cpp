#include "master.h"
#include <assert.h>
#include <windows.h>

float* Camera::worldviewMat = 0;
GLFWwindow* Camera::window = 0;
Vec3 Camera::pos = { 0, 3.1, 0 };
Vec3 Camera::renderPos = {0, 0, 0};
Vec3 Camera::rot = { 0, 0, 0 };
double Camera::prev_x = 0;
double Camera::prev_y = 0;
float Camera::yVel = 0;
std::thread* Camera::cameraThread = 0;
Vec3* Camera::poss = 0;
int Camera::possSize = 0;
int Camera::fence = 0;
bool Camera::grounded = false;

typedef struct VLKComputeContext {
	VkQueue computeQueue;

	VkBuffer inBuffer;
	VkBuffer outBuffer;

	VkDeviceMemory inMemory;
	VkDeviceMemory outMemory;

	VkShaderModule shader;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkCommandBuffer computeCmdBuffer;
	VkCommandPool commandPool;
} VLKComputeContext;

VLKComputeContext getComputeContext(VulkanRenderContext* vrc) {
	VLKComputeContext context = { };

	vkGetDeviceQueue(vrc->device.device, vrc->device.presentQueueIdx, 1, &context.computeQueue);

	VkPhysicalDeviceMemoryProperties properties;
	vkGetPhysicalDeviceMemoryProperties(vrc->device.physicalDevice, &properties);

	VkBufferCreateInfo inputBufferInfo = {};
	inputBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	inputBufferInfo.size = sizeof(float) * 3 * 3068;
	inputBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	inputBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	inputBufferInfo.queueFamilyIndexCount = 1;
	inputBufferInfo.pQueueFamilyIndices = &vrc->device.presentQueueIdx;

	VLKCheck(vkCreateBuffer(vrc->device.device, &inputBufferInfo, NULL, &context.inBuffer),
		"Could not create input buffer");

	VkDeviceSize memorySize = 3068 * 3 * sizeof(float);

	uint32_t memoryTypeIndex = VK_MAX_MEMORY_TYPES;
	for (uint32_t k = 0; k < properties.memoryTypeCount; k++) {
		if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & properties.memoryTypes[k].propertyFlags) &&
			(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & properties.memoryTypes[k].propertyFlags) &&
			(memorySize < properties.memoryHeaps[properties.memoryTypes[k].heapIndex].size)) {
			memoryTypeIndex = k;
			break;
		}
	}

	assert(memoryTypeIndex != VK_MAX_MEMORY_TYPES);

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = NULL;
	memoryAllocateInfo.allocationSize = memorySize;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

	VLKCheck(vkAllocateMemory(vrc->device.device, &memoryAllocateInfo, 0, &context.inMemory),
		"Could not allocate memory");

	void* mapped;
	VLKCheck(vkMapMemory(vrc->device.device, context.inMemory, 0, memorySize, 0, &mapped),
		"Could not map memory");

	memset(mapped, 0, 3068 * 3 * sizeof(float));

	vkUnmapMemory(vrc->device.device, context.inMemory);

	vkBindBufferMemory(vrc->device.device, context.inBuffer, context.inMemory, 0);

	VkBufferCreateInfo outputBufferInfo = {};
	outputBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	outputBufferInfo.size = sizeof(float) * 3068;
	outputBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	outputBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	outputBufferInfo.queueFamilyIndexCount = 1;
	outputBufferInfo.pQueueFamilyIndices = &vrc->device.presentQueueIdx;

	VLKCheck(vkCreateBuffer(vrc->device.device, &outputBufferInfo, NULL, &context.outBuffer),
		"Could not create input buffer");

	memorySize = 3068 * sizeof(float);

	memoryTypeIndex = VK_MAX_MEMORY_TYPES;
	for (uint32_t k = 0; k < properties.memoryTypeCount; k++) {
		if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & properties.memoryTypes[k].propertyFlags) &&
			(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & properties.memoryTypes[k].propertyFlags) &&
			(memorySize < properties.memoryHeaps[properties.memoryTypes[k].heapIndex].size)) {
			memoryTypeIndex = k;
			break;
		}
	}

	assert(memoryTypeIndex != VK_MAX_MEMORY_TYPES);

	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = NULL;
	memoryAllocateInfo.allocationSize = memorySize;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
	
	VLKCheck(vkAllocateMemory(vrc->device.device, &memoryAllocateInfo, 0, &context.outMemory),
		"Could not allocate memory");

	VLKCheck(vkMapMemory(vrc->device.device, context.outMemory, 0, memorySize, 0, &mapped),
		"Could not map memory");

	memset(mapped, 0, 3068 * sizeof(float));

	vkUnmapMemory(vrc->device.device, context.outMemory);

	vkBindBufferMemory(vrc->device.device, context.outBuffer, context.outMemory, 0);

	uint32_t codeSize;
	char* code = new char[20000];
	HANDLE fileHandle = 0;

	fileHandle = CreateFile("comp.spv", GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

	VLKCheck(vkCreateShaderModule(vrc->device.device, &vertexShaderCreationInfo, NULL, &context.shader),
		"Failed to create vertex shader module");

	delete[] code;

	VkDescriptorSetLayoutBinding bindings[2];
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	bindings[0].pImmutableSamplers = NULL;

	bindings[1].binding = 1;
	bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[1].descriptorCount = 1;
	bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	bindings[1].pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {};
	setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setLayoutCreateInfo.bindingCount = 2;
	setLayoutCreateInfo.pBindings = bindings;

	VLKCheck(vkCreateDescriptorSetLayout(vrc->device.device, &setLayoutCreateInfo, NULL, &context.descriptorSetLayout),
		"Failed to create DescriptorSetLayout");

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = NULL;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &context.descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

	VLKCheck(vkCreatePipelineLayout(vrc->device.device, &pipelineLayoutCreateInfo, NULL, &context.pipelineLayout),
		"Failed ot create pipline layout");

	VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
	shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageCreateInfo.module = context.shader;
	shaderStageCreateInfo.pName = "main";
	shaderStageCreateInfo.pSpecializationInfo = NULL;

	VkComputePipelineCreateInfo pipelineCreateInfo = {};
	pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineCreateInfo.pNext = NULL;
	pipelineCreateInfo.flags = 0;
	pipelineCreateInfo.layout = context.pipelineLayout;
	pipelineCreateInfo.stage = shaderStageCreateInfo;

	VLKCheck(vkCreateComputePipelines(vrc->device.device, NULL, 1, &pipelineCreateInfo, NULL, &context.pipeline),
		"Failed to create pipeline");

	return context;
}

static void cameraThreadRun(GLFWwindow* win, VulkanRenderContext* vrc) {
	std::vector<Cube*> closeCubes;
	Vec3* pvecs = NULL;

	VLKComputeContext context = getComputeContext(vrc);

	while (!glfwWindowShouldClose(win)) {
		
		while (Chunk::m_fence == 1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		Chunk::m_fence = Chunk::m_fence + 2;

		closeCubes.clear();
		for (int i = 0; i < Chunk::rcubesSize; i++) {
			Cube* cube = Chunk::rcubes[i];
			if (Camera::pos.dist(cube->m_pos) <= 8) {
				closeCubes.push_back(new Cube(cube));
			}
		}

		Chunk::m_fence = Chunk::m_fence - 2;

		int cbsz = closeCubes.size();
		Vec3* vecs = new Vec3[cbsz];

		for (int i = 0; i < cbsz; i++) {
			vecs[i] = { closeCubes[i]->m_pos.x, closeCubes[i]->m_pos.y, closeCubes[i]->m_pos.z };
		}

		Vertex* verts = new Vertex[cbsz];

		for (int i = 0; i < cbsz; i++) {
			verts[i].x = vecs[i].x;
			verts[i].y = vecs[i].y - 0.5f;
			verts[i].z = vecs[i].z;

			verts[i].tx = closeCubes[i]->tex->x;
			verts[i].ty = closeCubes[i]->tex->y;
			verts[i].tz = closeCubes[i]->tex->z;
			verts[i].rinf = closeCubes[i]->vid;
		}

		//Ray cast using Vulkan Compute shader

		while (Camera::fence > 0) {
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}

		Camera::fence = 1;

		Camera::poss = vecs;
		Camera::possSize = cbsz;

		Camera::fence = 0;

		/*
		if (ids != NULL) {
			delete[] ids;
		}

		if (infs != NULL) {
			delete[] infs;
		}

		if (verts != NULL) {
			delete[] verts;
		}
		*/

		if (pvecs != NULL) {
			delete[] pvecs;
		}

		pvecs = vecs;

		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}

void Camera::init(GLFWwindow* win, float* viewMat, VulkanRenderContext* vrc) {
	window = win;

	renderPos.x = pos.x;
	renderPos.y = -pos.y;
	renderPos.z = pos.z;

	worldviewMat = viewMat;
	getWorldview(worldviewMat, renderPos, rot);

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	prev_x = xpos;
	prev_y = ypos;

	cameraThread = new std::thread(cameraThreadRun, window, vrc);
}

bool hittingCube(Vec3 pos, Vec3 other) {
	if (pos.x + 0.1f < other.x - 0.5f || other.x + 0.5f  < pos.x - 0.1f) {
		return false;
	}

	if (pos.z + 0.1f < other.z - 0.5f || other.z + 0.5f  < pos.z - 0.1f) {
		return false;
	}

	if (pos.y < other.y - 0.5f || other.y + 0.5f  < pos.y - 1) {
		return false;
	}

	return true;
}

bool hittingCubes(Vec3 pos) {
	for (int i = 0; i < Camera::possSize; i++) {
		if (hittingCube(pos, Camera::poss[i])) {
			return true;
		}
	}

	return false;
}

void Camera::update(float dt) {
	bool focused = glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	if (focused) {
		float speed = 3.5f;

		if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
			speed *= 2;
		}

		float xVel = 0;
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

		//T = 3/5, H = 1.2

		if (grounded && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
			yVel = 10;
			grounded = false;
		}

		yVel -= 33.3333f * dt;

		while (fence == 1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		fence = fence + 2;

		pos.x += xVel;

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

		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

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

	renderPos.x = pos.x;
	renderPos.y = -pos.y;
	renderPos.z = pos.z;

	getWorldview(worldviewMat, renderPos, rot);
}

void Camera::destroy() {
	cameraThread->join();
}