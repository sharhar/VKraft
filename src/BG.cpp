//
//  BG.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#include "BG.h"

VKLDevice* BG::m_device = NULL;
VKLBuffer* BG::m_vertBuffer = NULL;
VKLShader* BG::m_shader = NULL;
VKLUniformObject* BG::m_uniform = NULL;
VKLPipeline* BG::m_pipeline = NULL;

void BG::init(VKLDevice* device, VKLSwapChain* swapChain, VKLFrameBuffer* framebuffer) {
	m_device = device;
	
	float backGroundVerts[] = {
		-1, -1, 0, 0,
		 1, -1, 1, 0,
		-1,  1, 0, 1,
		-1,  1, 0, 1,
		 1, -1, 1, 0,
		 1,  1, 1, 1
	};
	
	vklCreateStagedBuffer(device->deviceGraphicsContexts[0], &m_vertBuffer, backGroundVerts, 6 * 4 * sizeof(float), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	
	char* shaderPaths[2];
	shaderPaths[0] = "res/bg-vert.spv";
	shaderPaths[1] = "res/bg-frag.spv";

	VkShaderStageFlagBits stages[2];
	stages[0] = VK_SHADER_STAGE_VERTEX_BIT;
	stages[1] = VK_SHADER_STAGE_FRAGMENT_BIT;

	size_t offsets[2] = { 0, sizeof(float) * 2 };
	VkFormat formats[2] = { VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32_SFLOAT };

	VkDescriptorSetLayoutBinding bindings[1];
	bindings[0].binding = 0;
	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	bindings[0].descriptorCount = 1;
	bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	bindings[0].pImmutableSamplers = NULL;
	
	uint32_t vertexInputAttributeBindings[2] = {0, 0};

	VKLShaderCreateInfo shaderCreateInfo;
	memset(&shaderCreateInfo, 0, sizeof(VKLShaderCreateInfo));
	shaderCreateInfo.shaderPaths = shaderPaths;
	shaderCreateInfo.shaderStages = stages;
	shaderCreateInfo.shaderCount = 2;
	shaderCreateInfo.bindings = bindings;
	shaderCreateInfo.bindingsCount = 1;
	shaderCreateInfo.vertexInputAttributesCount = 2;
	shaderCreateInfo.vertexInputAttributeOffsets = offsets;
	shaderCreateInfo.vertexInputAttributeFormats = formats;
	shaderCreateInfo.vertexInputAttributeBindings = vertexInputAttributeBindings;
	
	VkVertexInputRate vertInputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	size_t stride = sizeof(float) * 4;
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
	
	vklSetUniformFramebuffer(m_device, m_uniform, framebuffer, 0);
}

void BG::render(VkCommandBuffer cmdBuffer) {
	VkDeviceSize vertexOffsets = 0;
	m_device->pvkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->pipeline);
	m_device->pvkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vertBuffer->buffer, &vertexOffsets);

	m_device->pvkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_pipeline->pipelineLayout, 0, 1, &m_uniform->descriptorSet, 0, NULL);

	m_device->pvkCmdDraw(cmdBuffer, 6, 1, 0, 0);
}

void BG::destroy() {
	vklDestroyPipeline(m_device, m_pipeline);
	vklDestroyUniformObject(m_device, m_uniform);
	vklDestroyShader(m_device, m_shader);
	vklDestroyBuffer(m_device, m_vertBuffer);
	
}
