#include "master.h"
#include <chrono>
#include <thread>
#include <windows.h>
#include "lodepng.h"

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

VLKShader* createBGShader(VLKDevice* device, char* vertPath, char* fragPath) {
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

	VkDescriptorSetLayoutBinding binding;
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	binding.pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {};
	setLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	setLayoutCreateInfo.bindingCount = 1;
	setLayoutCreateInfo.pBindings = &binding;

	VLKCheck(vkCreateDescriptorSetLayout(device->device, &setLayoutCreateInfo, NULL, &shader->setLayout),
		"Failed to create DescriptorSetLayout");

	VkDescriptorPoolSize uniformBufferPoolSize;
	uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	uniformBufferPoolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.maxSets = 1;
	poolCreateInfo.poolSizeCount = 1;
	poolCreateInfo.pPoolSizes = &uniformBufferPoolSize;

	VLKCheck(vkCreateDescriptorPool(device->device, &poolCreateInfo, NULL, &shader->descriptorPool),
		"Failed to create descriptor pool");

	VkDescriptorSetAllocateInfo descriptorAllocateInfo = {};
	descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorAllocateInfo.descriptorPool = shader->descriptorPool;
	descriptorAllocateInfo.descriptorSetCount = 1;
	descriptorAllocateInfo.pSetLayouts = &shader->setLayout;

	VLKCheck(vkAllocateDescriptorSets(device->device, &descriptorAllocateInfo, &shader->descriptorSet),
		"Failed to allocate descriptor sets");
	return shader;
}

void destroyBGShader(VLKDevice* device, VLKShader* shader) {
	vkDestroyDescriptorPool(device->device, shader->descriptorPool, NULL);
	vkDestroyDescriptorSetLayout(device->device, shader->setLayout, NULL);

	vkDestroyShaderModule(device->device, shader->vertexShaderModule, NULL);
	vkDestroyShaderModule(device->device, shader->fragmentShaderModule, NULL);

	free(shader);
}

VLKShader* createCursorShader(VLKDevice* device, char* vertPath, char* fragPath, void* uniformBuffer, uint32_t uniformSize) {
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
	VLKCheck(vkMapMemory(device->device, shader->memory, 0, VK_WHOLE_SIZE, 0, &matrixMapped),
		"Failed to map uniform buffer memory.");

	memcpy(matrixMapped, uniformBuffer, uniformSize);

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

void destroyCursorShader(VLKDevice* device, VLKShader* shader) {
	vkFreeMemory(device->device, shader->memory, NULL);

	vkDestroyDescriptorPool(device->device, shader->descriptorPool, NULL);
	vkDestroyDescriptorSetLayout(device->device, shader->setLayout, NULL);

	vkDestroyBuffer(device->device, shader->buffer, NULL);

	vkDestroyShaderModule(device->device, shader->vertexShaderModule, NULL);
	vkDestroyShaderModule(device->device, shader->fragmentShaderModule, NULL);

	free(shader);
}

VLKPipeline* createBGPipeline(VLKDevice* device, VLKSwapchain* swapChain, VLKShader* shader) {
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
	vertexBindingDescription.stride = sizeof(float) * 4;
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertexAttributeDescritpion[2];
	vertexAttributeDescritpion[0].location = 0;
	vertexAttributeDescritpion[0].binding = 0;
	vertexAttributeDescritpion[0].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescritpion[0].offset = 0;

	vertexAttributeDescritpion[1].location = 1;
	vertexAttributeDescritpion[1].binding = 0;
	vertexAttributeDescritpion[1].format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescritpion[1].offset = 2 * sizeof(float);

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

void destroyBGPipeline(VLKDevice* device, VLKPipeline* pipeline) {
	vkDestroyPipeline(device->device, pipeline->pipeline, NULL);
	vkDestroyPipelineLayout(device->device, pipeline->pipelineLayout, NULL);

	free(pipeline);
}

VLKPipeline* createCursorPipeline(VLKDevice* device, VLKSwapchain* swapChain, VLKShader* shader) {
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
	vertexBindingDescription.stride = sizeof(float) * 2;
	vertexBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription vertexAttributeDescritpion;
	vertexAttributeDescritpion.location = 0;
	vertexAttributeDescritpion.binding = 0;
	vertexAttributeDescritpion.format = VK_FORMAT_R32G32_SFLOAT;
	vertexAttributeDescritpion.offset = 0;

	VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {};
	vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexBindingDescriptions = &vertexBindingDescription;
	vertexInputStateCreateInfo.vertexAttributeDescriptionCount = 1;
	vertexInputStateCreateInfo.pVertexAttributeDescriptions = &vertexAttributeDescritpion;

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

void destroyCursorPipeline(VLKDevice* device, VLKPipeline* pipeline) {
	vkDestroyPipeline(device->device, pipeline->pipeline, NULL);
	vkDestroyPipelineLayout(device->device, pipeline->pipelineLayout, NULL);

	free(pipeline);
}

VLKTexture* createCursorTexture(VLKDevice* device, char* path) {
	VLKTexture* texture = (VLKTexture*)malloc(sizeof(VLKTexture));

	texture->width = 48;
	texture->height = 48;

	uint32_t* data = new uint32_t[48*48];

	for (int y = 0; y < 48;y++) {
		for (int x = 0; x < 48; x++) {
			if ((x < 20 || x > 28) && (y < 20 || y > 28)) {
				data[y * 48 + x] = 0x00000000;
			} else if (((x == 20 || x == 28) && (y <= 20 || y >= 28)) ||
					   ((x <= 20 || x >= 28) && (y == 20 || y == 28)) ||
					   (x == 47 || x == 0 || y == 47 || y == 0)) {
				data[y * 48 + x] = 0xff000000;
			} else {
				data[y * 48 + x] = 0xffffffff;
			}
		}
	}

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

	memcpy(imageMapped, data, texture->width * texture->height* sizeof(uint32_t));

	VkMappedMemoryRange memoryRange = {};
	memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	memoryRange.memory = texture->textureImageMemory;
	memoryRange.offset = 0;
	memoryRange.size = VK_WHOLE_SIZE;
	vkFlushMappedMemoryRanges(device->device, 1, &memoryRange);

	vkUnmapMemory(device->device, texture->textureImageMemory);

	delete[] data;

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
	descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

	texture->descriptorImageInfo = descriptorImageInfo;

	return texture;
}

void destroyCursorTexture(VLKDevice* device, VLKTexture* texture) {
	vkFreeMemory(device->device, texture->textureImageMemory, NULL);

	vkDestroySampler(device->device, texture->sampler, NULL);
	vkDestroyImageView(device->device, texture->textureView, NULL);
	vkDestroyImage(device->device, texture->textureImage, NULL);

	free(texture);
}

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(1280, 720, "VKraft", NULL, NULL);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowFocusCallback(window, window_focus_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	VLKContext* context = vlkCreateContext();
	VLKDevice* device;
	VLKSwapchain* swapChain;
	vlkCreateDeviceAndSwapchain(window, context, &device, &swapChain);

	CubeUniformBuffer uniformBuffer;
	memcpy(uniformBuffer.proj, getPerspective(), sizeof(float) * 16);

	VLKShader* shader = vlkCreateShader(device, "cube-vert.spv", "cube-frag.spv", &uniformBuffer, sizeof(CubeUniformBuffer));
	VLKPipeline* pipeline = vlkCreatePipeline(device, swapChain, shader);
	
	VLKFramebuffer* frameBuffer = vlkCreateFramebuffer(device, swapChain->imageCount, swapChain->width, swapChain->height);

	Vec3 pos = {0, 0, 1};
	Vec3 rot = {0, 0, 0};
	double prev_x = 0;
	double prev_y = 0;

	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);

	prev_x = xpos;
	prev_y = ypos;
	
	VulkanRenderContext renderContext = {};
	renderContext.device = device;
	renderContext.swapChain = swapChain;
	renderContext.uniformBuffer = &uniformBuffer;
	renderContext.shader = shader;
	renderContext.pipeline = pipeline;
	renderContext.framebuffer = frameBuffer;

	Camera::init(window, uniformBuffer.view, &renderContext);

	Chunk::init(245325, window, &renderContext);

	VLKTexture* texture = vlkCreateTexture(device, "pack.png");
	vlkBindTexture(device, shader, texture);

	float cursorVerts[] = {
		-0.03125f, -0.003125f,
		 0.03125f, -0.003125f,
		-0.03125f,  0.003125f,
		-0.03125f,  0.003125f,
		 0.03125f, -0.003125f,
		 0.03125f,  0.003125f,
		
		-0.003125f, -0.03125f,
		 0.003125f, -0.03125f,
		-0.003125f,  0.03125f,
		-0.003125f,  0.03125f,
		 0.003125f, -0.03125f,
		 0.003125f,  0.03125f };

	float backGroundVerts[] = {
		-1, -1, 0, 0,
		 1, -1, 1, 0,
		-1,  1, 0, 1,
		-1,  1, 0, 1,
		 1, -1, 1, 0,
		 1,  1, 1, 1 };

	float aspect = 16.0f / 9.0f;

	VLKModel* bgModel = vlkCreateModel(device, backGroundVerts, 6 * 4 * sizeof(float));
	VLKShader* bgShader = createBGShader(device, "bg-vert.spv", "bg-frag.spv");
	VLKPipeline* bgPipeline = createBGPipeline(device, swapChain, bgShader);

	VkWriteDescriptorSet writeDescriptor = {};
	writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptor.dstSet = bgShader->descriptorSet;
	writeDescriptor.dstBinding = 0;
	writeDescriptor.dstArrayElement = 0;
	writeDescriptor.descriptorCount = 1;
	writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptor.pImageInfo = &frameBuffer->descriptorImageInfo;
	writeDescriptor.pBufferInfo = NULL;
	writeDescriptor.pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device->device, 1, &writeDescriptor, 0, NULL);

	VLKModel* cursorModel = vlkCreateModel(device, cursorVerts, 12 * 2 * sizeof(float));
	VLKShader* cursorShader = createCursorShader(device, "cursor-vert.spv", "cursor-frag.spv", &aspect, sizeof(float));
	VLKPipeline* cursorPipeline = createCursorPipeline(device, swapChain, cursorShader);
	VLKTexture* cursorTexture = createCursorTexture(device, "Cursor.png");

	writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptor.dstSet = cursorShader->descriptorSet;
	writeDescriptor.dstBinding = 1;
	writeDescriptor.dstArrayElement = 0;
	writeDescriptor.descriptorCount = 1;
	writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptor.pImageInfo = &frameBuffer->descriptorImageInfo;
	writeDescriptor.pBufferInfo = NULL;
	writeDescriptor.pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device->device, 1, &writeDescriptor, 0, NULL);

	double ct = glfwGetTime();
	double dt = 0;

	double cts = glfwGetTime();
	double dts = 0;

	std::cout << sizeof(RCube) << "\n";

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		dts = glfwGetTime() - cts;

		std::this_thread::sleep_for(std::chrono::nanoseconds((long)((1.0f/120.0f - dts - 0.001) * 1000000000L)));

		cts = glfwGetTime();

		dt = glfwGetTime() - ct;
		ct = glfwGetTime();

		Camera::update(dt);

		VkViewport viewport = { 0, 0, swapChain->width, swapChain->height, 0, 1 };
		VkRect2D scissor = { 0, 0, swapChain->width, swapChain->height };
		VkDeviceSize offsets = {};

		vlkStartFramebuffer(device, frameBuffer);

		Chunk::render(device, swapChain);

		vlkEndFramebuffer(device, frameBuffer);
		
		vlkClear(device, swapChain);

		vkCmdSetViewport(device->drawCmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(device->drawCmdBuffer, 0, 1, &scissor);
		
		vkCmdBindPipeline(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bgPipeline->pipeline);
		vkCmdBindVertexBuffers(device->drawCmdBuffer, 0, 1, &bgModel->vertexInputBuffer, &offsets);
		vkCmdBindDescriptorSets(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			bgPipeline->pipelineLayout, 0, 1, &bgShader->descriptorSet, 0, NULL);

		vkCmdDraw(device->drawCmdBuffer, 6, 1, 0, 0);

		vkCmdSetViewport(device->drawCmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(device->drawCmdBuffer, 0, 1, &scissor);

		vkCmdBindPipeline(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, cursorPipeline->pipeline);
		vkCmdBindVertexBuffers(device->drawCmdBuffer, 0, 1, &cursorModel->vertexInputBuffer, &offsets);
		vkCmdBindDescriptorSets(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			cursorPipeline->pipelineLayout, 0, 1, &cursorShader->descriptorSet, 0, NULL);

		vkCmdDraw(device->drawCmdBuffer, 12, 1, 0, 0);
		
		vlkSwap(device, swapChain);
	}

	destroyCursorTexture(device, cursorTexture);
	destroyCursorPipeline(device, cursorPipeline);
	destroyCursorShader(device, cursorShader);
	vlkDestroyModel(device, cursorModel);

	destroyBGPipeline(device, bgPipeline);
	destroyBGShader(device, bgShader);
	vlkDestroyModel(device,bgModel);

	Chunk::destroy(device);
	Camera::destroy(device);

	vlkDestroyFramebuffer(device, frameBuffer);
	vlkDestroyTexture(device, texture);
	vlkDestroyPipeline(device, pipeline);
	vlkDestroyShader(device, shader);
	vlkDestroyDeviceAndSwapchain(context, device, swapChain);
	vlkDestroyContext(context);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}