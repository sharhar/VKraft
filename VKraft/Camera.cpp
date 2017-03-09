#include "master.h"
#include <assert.h>

float* Camera::worldviewMat = 0;
GLFWwindow* Camera::window = 0;
Vec3 Camera::pos = { 0, 3.1, 0 };
Vec3 Camera::renderPos = { 0, 0, 0 };
Vec3 Camera::rot = { 0, 0, 0 };
double Camera::prev_x = 0;
double Camera::prev_y = 0;
float Camera::yVel = 0;
std::thread* Camera::cameraThread = 0;
Vec3* Camera::poss = 0;
int Camera::possSize = 0;
int Camera::fence = 0;
bool Camera::grounded = false;
VLKComputeContext Camera::context = {};

VLKComputeContext getComputeContext(VulkanRenderContext* vrc) {
	VLKComputeContext context = {};

	vkGetDeviceQueue(vrc->device->device, vrc->device->presentQueueIdx, 1, &context.computeQueue);

	VkPhysicalDeviceMemoryProperties properties;
	vkGetPhysicalDeviceMemoryProperties(vrc->device->physicalDevice, &properties);

	VkBufferCreateInfo posBufferInfo = {};
	posBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	posBufferInfo.size = sizeof(float) * 6;
	posBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	posBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	posBufferInfo.queueFamilyIndexCount = 1;
	posBufferInfo.pQueueFamilyIndices = &vrc->device->presentQueueIdx;

	VLKCheck(vkCreateBuffer(vrc->device->device, &posBufferInfo, NULL, &context.posBuffer),
		"Could not create input buffer");

	VkDeviceSize memorySize = 6 * sizeof(float);

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

	VLKCheck(vkAllocateMemory(vrc->device->device, &memoryAllocateInfo, 0, &context.posMemory),
		"Could not allocate memory");

	void* mapped;
	VLKCheck(vkMapMemory(vrc->device->device, context.posMemory, 0, memorySize, 0, &mapped),
		"Could not map memory");

	memset(mapped, 0, 6 * sizeof(float));

	vkUnmapMemory(vrc->device->device, context.posMemory);

	vkBindBufferMemory(vrc->device->device, context.posBuffer, context.posMemory, 0);
	
	std::vector<char> shaderCode = readFile("raycast-comp.spv");

	VkShaderModuleCreateInfo vertexShaderCreationInfo = {};
	vertexShaderCreationInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vertexShaderCreationInfo.codeSize = shaderCode.size();
	vertexShaderCreationInfo.pCode = (uint32_t *)shaderCode.data();

	VLKCheck(vkCreateShaderModule(vrc->device->device, &vertexShaderCreationInfo, NULL, &context.shader),
		"Failed to create vertex shader module");

	shaderCode.clear();

	VkDescriptorSetLayoutBinding bindings[3];
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

	bindings[2].binding = 2;
	bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	bindings[2].descriptorCount = 1;
	bindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	bindings[2].pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {};
	setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setLayoutCreateInfo.bindingCount = 3;
	setLayoutCreateInfo.pBindings = bindings;

	VLKCheck(vkCreateDescriptorSetLayout(vrc->device->device, &setLayoutCreateInfo, NULL, &context.descriptorSetLayout),
		"Failed to create DescriptorSetLayout");

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = NULL;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &context.descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

	VLKCheck(vkCreatePipelineLayout(vrc->device->device, &pipelineLayoutCreateInfo, NULL, &context.pipelineLayout),
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

	VLKCheck(vkCreateComputePipelines(vrc->device->device, NULL, 1, &pipelineCreateInfo, NULL, &context.pipeline),
		"Failed to create pipeline");

	VkDescriptorPoolSize descriptorPoolSize = {};
	descriptorPoolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorPoolSize.descriptorCount = 3;

	VkDescriptorPoolCreateInfo descriptorCreateInfo = {};
	descriptorCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorCreateInfo.pNext = NULL;
	descriptorCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptorCreateInfo.maxSets = 1;
	descriptorCreateInfo.poolSizeCount = 1;
	descriptorCreateInfo.pPoolSizes = &descriptorPoolSize;

	VLKCheck(vkCreateDescriptorPool(vrc->device->device, &descriptorCreateInfo, 0, &context.descriptorPool),
		"Failed to create discriptor pool");

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = NULL;
	descriptorSetAllocateInfo.descriptorPool = context.descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = &context.descriptorSetLayout;

	VLKCheck(vkAllocateDescriptorSets(vrc->device->device, &descriptorSetAllocateInfo, &context.descriptorSet),
		"Failed to create descriptor set");

	VkDescriptorBufferInfo posDescriptorBuffer = {};
	posDescriptorBuffer.buffer = context.posBuffer;
	posDescriptorBuffer.offset = 0;
	posDescriptorBuffer.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet writeDescriptorSet = {};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.pNext = NULL;
	writeDescriptorSet.dstSet = context.descriptorSet;
	writeDescriptorSet.dstBinding = 2;
	writeDescriptorSet.dstArrayElement = 0;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	writeDescriptorSet.pBufferInfo = &posDescriptorBuffer;
	writeDescriptorSet.pImageInfo = NULL;
	writeDescriptorSet.pTexelBufferView = NULL;

	vkUpdateDescriptorSets(vrc->device->device, 1, &writeDescriptorSet, 0, 0);

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.pNext = NULL;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = vrc->device->presentQueueIdx;

	VLKCheck(vkCreateCommandPool(vrc->device->device, &commandPoolCreateInfo, NULL, &context.commandPool),
		"Failed to create command pool");

	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.pNext = NULL;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = context.commandPool;
	commandBufferAllocateInfo.commandBufferCount = 1;

	VLKCheck(vkAllocateCommandBuffers(vrc->device->device, &commandBufferAllocateInfo, &context.computeCmdBuffer),
		"Failed to create command pool");

	return context;
}

void destroyComputeContext(VLKDevice* device, VLKComputeContext& context) {
	vkFreeCommandBuffers(device->device, context.commandPool, 1, &context.computeCmdBuffer);
	vkDestroyCommandPool(device->device, context.commandPool, NULL);

	vkFreeDescriptorSets(device->device, context.descriptorPool, 1, &context.descriptorSet);
	vkDestroyDescriptorPool(device->device, context.descriptorPool, NULL);
	vkDestroyDescriptorSetLayout(device->device, context.descriptorSetLayout, NULL);

	vkDestroyPipeline(device->device, context.pipeline, NULL);
	vkDestroyPipelineLayout(device->device, context.pipelineLayout, NULL);

	vkDestroyShaderModule(device->device, context.shader, NULL);

	vkFreeMemory(device->device, context.posMemory, NULL);

	vkDestroyBuffer(device->device, context.posBuffer, NULL);
}

static void cameraThreadRun(GLFWwindow* win, VulkanRenderContext* vrc, VLKComputeContext* context) {
	std::vector<Cube*> closeCubes;
	Vec3* pvecs = NULL;

	while (!glfwWindowShouldClose(win)) {

		while (Chunk::m_fence == 1) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		Chunk::m_fence = Chunk::m_fence + 2;

		closeCubes.clear();
		for (int i = 0; i < Chunk::rcubesSize; i++) {
			Cube cube = Chunk::rcubes[i];
			if (Camera::pos.dist(cube.pos) <= 8) {
				Cube* temp = (Cube*)malloc(sizeof(Cube));
				temp->pos.x = cube.pos.x;
				temp->pos.y = cube.pos.y;
				temp->pos.z = cube.pos.z;
				temp->type = cube.type;
				temp->vid = cube.vid;

				closeCubes.push_back(temp);
			}
		}

		Chunk::m_fence = Chunk::m_fence - 2;

		int cbsz = closeCubes.size();
		Vec3* vecs = new Vec3[cbsz];

		for (int i = 0; i < cbsz; i++) {
			vecs[i] = { closeCubes[i]->pos.x, closeCubes[i]->pos.y, closeCubes[i]->pos.z };
		}

		if (cbsz > 0) {
			float tx = Camera::rot.x;
			float ty = Camera::rot.y;

			float nx = sin(ty*DEG_TO_RAD)*cos(tx*DEG_TO_RAD);
			float ny = sin(tx*DEG_TO_RAD);
			float nz = -cos(ty*DEG_TO_RAD)*cos(tx*DEG_TO_RAD);

			void *mapped;
			vkMapMemory(vrc->device->device, context->posMemory, 0, VK_WHOLE_SIZE, 0, &mapped);

			float* posData = (float*)mapped;
			posData[0] = Camera::pos.x;
			posData[1] = Camera::pos.y + 0.5f;
			posData[2] = Camera::pos.z;
			posData[3] = nx;
			posData[4] = ny;
			posData[5] = nz;

			VkMappedMemoryRange memoryRange = {};
			memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			memoryRange.memory = context->posMemory;
			memoryRange.offset = 0;
			memoryRange.size = VK_WHOLE_SIZE;
			vkFlushMappedMemoryRanges(vrc->device->device, 1, &memoryRange);

			vkUnmapMemory(vrc->device->device, context->posMemory);

			VkPhysicalDeviceMemoryProperties properties;
			vkGetPhysicalDeviceMemoryProperties(vrc->device->physicalDevice, &properties);

			VkBufferCreateInfo inputBufferInfo = {};
			inputBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			inputBufferInfo.size = sizeof(float) * 4 * 3068;
			inputBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			inputBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			inputBufferInfo.queueFamilyIndexCount = 1;
			inputBufferInfo.pQueueFamilyIndices = &vrc->device->presentQueueIdx;

			VLKCheck(vkCreateBuffer(vrc->device->device, &inputBufferInfo, NULL, &context->inBuffer),
				"Could not create input buffer");

			VkDeviceSize memorySize = 3068 * 4 * sizeof(float);

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

			VLKCheck(vkAllocateMemory(vrc->device->device, &memoryAllocateInfo, 0, &context->inMemory),
				"Could not allocate memory");

			VLKCheck(vkMapMemory(vrc->device->device, context->inMemory, 0, memorySize, 0, &mapped),
				"Could not map memory");

			float* inputData = (float*)mapped;

			for (int i = 0; i < cbsz; i++) {
				inputData[i * 4 + 0] = vecs[i].x;
				inputData[i * 4 + 1] = vecs[i].y;
				inputData[i * 4 + 2] = vecs[i].z;
				inputData[i * 4 + 3] = (float)closeCubes[i]->vid;
			}

			vkUnmapMemory(vrc->device->device, context->inMemory);

			vkBindBufferMemory(vrc->device->device, context->inBuffer, context->inMemory, 0);

			VkBufferCreateInfo outputBufferInfo = {};
			outputBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			outputBufferInfo.size = sizeof(float) * 3068;
			outputBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			outputBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			outputBufferInfo.queueFamilyIndexCount = 1;
			outputBufferInfo.pQueueFamilyIndices = &vrc->device->presentQueueIdx;

			VLKCheck(vkCreateBuffer(vrc->device->device, &outputBufferInfo, NULL, &context->outBuffer),
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

			VLKCheck(vkAllocateMemory(vrc->device->device, &memoryAllocateInfo, 0, &context->outMemory),
				"Could not allocate memory");

			VLKCheck(vkMapMemory(vrc->device->device, context->outMemory, 0, memorySize, 0, &mapped),
				"Could not map memory");

			memset(mapped, 0, 3068 * sizeof(float));

			vkUnmapMemory(vrc->device->device, context->outMemory);

			vkBindBufferMemory(vrc->device->device, context->outBuffer, context->outMemory, 0);

			VkDescriptorBufferInfo inDescriptorBuffer = {};
			inDescriptorBuffer.buffer = context->inBuffer;
			inDescriptorBuffer.offset = 0;
			inDescriptorBuffer.range = VK_WHOLE_SIZE;

			VkDescriptorBufferInfo outDescriptorBuffer = {};
			outDescriptorBuffer.buffer = context->outBuffer;
			outDescriptorBuffer.offset = 0;
			outDescriptorBuffer.range = VK_WHOLE_SIZE;

			VkWriteDescriptorSet writeDescriptorSet[2] = {};
			writeDescriptorSet[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet[0].pNext = NULL;
			writeDescriptorSet[0].dstSet = context->descriptorSet;
			writeDescriptorSet[0].dstBinding = 0;
			writeDescriptorSet[0].dstArrayElement = 0;
			writeDescriptorSet[0].descriptorCount = 1;
			writeDescriptorSet[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSet[0].pBufferInfo = &inDescriptorBuffer;
			writeDescriptorSet[0].pImageInfo = NULL;
			writeDescriptorSet[0].pTexelBufferView = NULL;

			writeDescriptorSet[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet[1].pNext = NULL;
			writeDescriptorSet[1].dstSet = context->descriptorSet;
			writeDescriptorSet[1].dstBinding = 1;
			writeDescriptorSet[1].dstArrayElement = 0;
			writeDescriptorSet[1].descriptorCount = 1;
			writeDescriptorSet[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSet[1].pBufferInfo = &outDescriptorBuffer;
			writeDescriptorSet[1].pImageInfo = NULL;
			writeDescriptorSet[1].pTexelBufferView = NULL;

			vkUpdateDescriptorSets(vrc->device->device, 2, writeDescriptorSet, 0, 0);

			vkResetCommandBuffer(context->computeCmdBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

			VkCommandBufferBeginInfo commandBufferBeginInfo = {};
			commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			commandBufferBeginInfo.pNext = NULL;
			commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			commandBufferBeginInfo.pInheritanceInfo = NULL;

			VLKCheck(vkBeginCommandBuffer(context->computeCmdBuffer, &commandBufferBeginInfo),
				"Failed to begin command buffer");

			vkCmdBindPipeline(context->computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, context->pipeline);

			vkCmdBindDescriptorSets(context->computeCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
				context->pipelineLayout, 0, 1, &context->descriptorSet, 0, 0);

			vkCmdDispatch(context->computeCmdBuffer, cbsz, 1, 1);

			VLKCheck(vkEndCommandBuffer(context->computeCmdBuffer),
				"Failed to end command buffer");

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.pNext = NULL;
			submitInfo.pSignalSemaphores = NULL;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = NULL;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitDstStageMask = NULL;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &context->computeCmdBuffer;
			VLKCheck(vkQueueSubmit(context->computeQueue, 1, &submitInfo, VK_NULL_HANDLE),
				"Failed to submit Queue");
			vkQueueWaitIdle(context->computeQueue);

			vkMapMemory(vrc->device->device, context->outMemory, 0, VK_WHOLE_SIZE, 0, &mapped);

			float* data = (float*)mapped;

			float closest = 10;
			int id = -1;

			for (int i = 0; i < cbsz; i++) {
				if (data[i] != -1 && data[i] < closest) {
					closest = data[i];
					id = i;
				}
			}

			if (id != -1) {
				vrc->uniformBuffer->selected.x = vecs[id].x;
				vrc->uniformBuffer->selected.y = vecs[id].y;
				vrc->uniformBuffer->selected.z = vecs[id].z;
			}
			else {
				vrc->uniformBuffer->selected.x = 0.25f;
				vrc->uniformBuffer->selected.y = 0.25f;
				vrc->uniformBuffer->selected.z = 0.25f;
			}

			vkUnmapMemory(vrc->device->device, context->outMemory);

			vkFreeMemory(vrc->device->device, context->inMemory, NULL);
			vkFreeMemory(vrc->device->device, context->outMemory, NULL);

			vkDestroyBuffer(vrc->device->device, context->inBuffer, NULL);
			vkDestroyBuffer(vrc->device->device, context->outBuffer, NULL);
		}

		while (Camera::fence > 0) {
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}

		Camera::fence = 1;

		Camera::poss = vecs;
		Camera::possSize = cbsz;

		Camera::fence = 0;

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

	context = getComputeContext(vrc);

	cameraThread = new std::thread(cameraThreadRun, window, vrc, &context);
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

void Camera::destroy(VLKDevice* device) {
	cameraThread->join();

	destroyComputeContext(device, context);
}