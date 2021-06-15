//
//  Cursor.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#include "Cursor.h"

VKLDevice* Cursor::m_device = NULL;
VKLBuffer* Cursor::m_vertBuffer = NULL;
VKLBuffer* Cursor::m_uniformBuffer = NULL;
VKLShader* Cursor::m_shader = NULL;
VKLUniformObject* Cursor::m_uniform = NULL;
VKLPipeline* Cursor::m_pipeline = NULL;

void Cursor::init(VKLDevice* device, VKLSwapChain* swapChain, VKLFrameBuffer* framebuffer) {
	m_device = device;
	
	float cursorVerts[] = {
		-25, -2.5f,
		 25, -2.5f,
		-25,  2.5f,
		-25,  2.5f,
		 25, -2.5f,
		 25,  2.5f,

		-2.5f, -25,
		 2.5f, -25,
		-2.5f,  25,
		-2.5f,  25,
		 2.5f, -25,
		 2.5f,  25 };
	
	vklCreateStagedBuffer(device->deviceGraphicsContexts[0], &m_vertBuffer, cursorVerts, 12 * 2 * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	
	char* shaderPaths[2];
	shaderPaths[0] = "res/cursor-vert.spv";
	shaderPaths[1] = "res/cursor-frag.spv";

	VkShaderStageFlagBits stages[2];
	stages[0] = VK_SHADER_STAGE_VERTEX_BIT;
	stages[1] = VK_SHADER_STAGE_FRAGMENT_BIT;

	size_t offsets[1] = { 0 };
	VkFormat formats[1] = { VK_FORMAT_R32G32_SFLOAT };

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
	
	uint32_t vertexInputAttributeBinding = 0;

	VKLShaderCreateInfo shaderCreateInfo;
	memset(&shaderCreateInfo, 0, sizeof(VKLShaderCreateInfo));
	shaderCreateInfo.shaderPaths = shaderPaths;
	shaderCreateInfo.shaderStages = stages;
	shaderCreateInfo.shaderCount = 2;
	shaderCreateInfo.bindings = bindings;
	shaderCreateInfo.bindingsCount = 2;
	//shaderCreateInfo.vertexInputAttributeStride = sizeof(float) * 2;
	shaderCreateInfo.vertexInputAttributesCount = 1;
	shaderCreateInfo.vertexInputAttributeOffsets = offsets;
	shaderCreateInfo.vertexInputAttributeFormats = formats;
	shaderCreateInfo.vertexInputAttributeBindings = &vertexInputAttributeBinding;
	
	VkVertexInputRate vertInputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	size_t stride = sizeof(float) * 2;
	shaderCreateInfo.vertexBindingsCount = 1;
	shaderCreateInfo.vertexBindingInputRates = &vertInputRate;
	shaderCreateInfo.vertexBindingStrides = &stride;
	
	vklCreateShader(device, &m_shader, &shaderCreateInfo);
	
	VKLGraphicsPipelineCreateInfo pipelineCreateInfo;
	memset(&pipelineCreateInfo, 0, sizeof(VKLGraphicsPipelineCreateInfo));
	pipelineCreateInfo.shader = m_shader;
	pipelineCreateInfo.renderPass = swapChain->backBuffer->renderPass;
	pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineCreateInfo.extent.width = swapChain->width;
	pipelineCreateInfo.extent.height = swapChain->height;

	vklCreateGraphicsPipeline(device, &m_pipeline, &pipelineCreateInfo);
	
	vklCreateUniformObject(device, &m_uniform, m_shader);
	
	vklCreateBuffer(device, &m_uniformBuffer, VK_FALSE, sizeof(float) * 18, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	
	vklSetUniformBuffer(m_device, m_uniform, m_uniformBuffer, 0);
	vklSetUniformFramebuffer(m_device, m_uniform, framebuffer, 1);
	
	updateProjection(swapChain->width, swapChain->height);
}

void Cursor::updateProjection(int width, int height) {
	float r = width;
	float l = 0;
	float t = height;
	float b = 0;
	float f = 1;
	float n = -1;

	float uniformData[] = {
		2 / (r - l), 0, 0, 0,
		0, 2 / (t - b), 0, 0,
		0, 0, -2 / (f - n), 0,
		-(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1,

		width/2, height/2
	};
	
	vklWriteToMemory(m_device, m_uniformBuffer->memory, uniformData, sizeof(float) * 18, 0);
}

void Cursor::rebuildPipeline(VKLSwapChain* swapChain, VKLFrameBuffer* framebuffer) {
	vklDestroyPipeline(m_device, m_pipeline);
	
	VKLGraphicsPipelineCreateInfo pipelineCreateInfo;
	memset(&pipelineCreateInfo, 0, sizeof(VKLGraphicsPipelineCreateInfo));
	pipelineCreateInfo.shader = m_shader;
	pipelineCreateInfo.renderPass = swapChain->backBuffer->renderPass;
	pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineCreateInfo.extent.width = swapChain->width;
	pipelineCreateInfo.extent.height = swapChain->height;

	vklCreateGraphicsPipeline(m_device, &m_pipeline, &pipelineCreateInfo);
	
	vklSetUniformFramebuffer(m_device, m_uniform, framebuffer, 1);
}

void Cursor::render(VkCommandBuffer cmdBuffer) {
	VkDeviceSize vertexOffsets = 0;
	m_device->pvkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->pipeline);
	m_device->pvkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vertBuffer->buffer, &vertexOffsets);

	m_device->pvkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipeline->pipelineLayout, 0, 1, &m_uniform->descriptorSet, 0, NULL);

	m_device->pvkCmdDraw(cmdBuffer, 12, 1, 0, 0);
}

void Cursor::destroy() {
	vklDestroyPipeline(m_device, m_pipeline);
	vklDestroyUniformObject(m_device, m_uniform);
	vklDestroyShader(m_device, m_shader);
	vklDestroyBuffer(m_device, m_vertBuffer);
	vklDestroyBuffer(m_device, m_uniformBuffer);
}
