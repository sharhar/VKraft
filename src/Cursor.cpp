//
//  Cursor.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#include "Cursor.h"
#include "Utils.h"

Cursor::Cursor(const VKLDevice* device, VKLRenderTarget* renderTarget, VKLQueue* queue) {
	m_device = device;
	m_queue = queue;
	
	
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
	 
	VKLBufferCreateInfo vertBufferCreateInfo;
	vertBufferCreateInfo.setDevice(device).setSize(12 * 2 * sizeof(float))
						.setMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
						.setUsage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	
	m_vertBuffer.build(vertBufferCreateInfo);
	m_vertBuffer.uploadData(queue, cursorVerts, 12 * 2 * sizeof(float), 0);
	
	size_t vertSize = 0;
	uint32_t* vertCode = (uint32_t*)readBinaryFile("res/cursor-vert.spv", &vertSize);
	
	size_t fragSize = 0;
	uint32_t* fragCode = (uint32_t*)readBinaryFile("res/cursor-frag.spv", &fragSize);
	
	VKLShaderCreateInfo shaderCreateInfo;
	shaderCreateInfo.setDevice(device)
					.addShaderModule(vertCode, vertSize, VK_SHADER_STAGE_VERTEX_BIT, "main")
					.addShaderModule(fragCode, fragSize, VK_SHADER_STAGE_FRAGMENT_BIT, "main")
					.addVertexInputBinding(0)
						.setStride(sizeof(float) * 2)
						.addAttrib(0, VK_FORMAT_R32G32_SFLOAT, 0)
					.end()
					.addDescriptorSet()
						.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
					.end()
					.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2);
	
	m_shader.build(shaderCreateInfo);
	
	VkDescriptorPoolSize poolSize;
	poolSize.descriptorCount = 1;
	poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
	memset(&descriptorPoolCreateInfo, 0, sizeof(VkDescriptorPoolCreateInfo));
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.poolSizeCount = 1;
	descriptorPoolCreateInfo.pPoolSizes = &poolSize;

	VK_CALL(m_device->vk.CreateDescriptorPool(device->handle(), &descriptorPoolCreateInfo,
											  device->allocationCallbacks(), &m_pool));
	
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
	memset(&descriptorSetAllocateInfo, 0, sizeof(VkDescriptorSetAllocateInfo));
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = m_pool;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pSetLayouts = m_shader.getDescriptorSetLayouts();

	VK_CALL(m_device->vk.AllocateDescriptorSets(device->handle(), &descriptorSetAllocateInfo, &m_descSet));
	
	VKLImageCreateInfo tempImageCreateInfo;
	tempImageCreateInfo.setDevice(m_device).setExtent(4, 4, 1)
						.setFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
						.setUsage(VK_IMAGE_USAGE_SAMPLED_BIT)
						.setMemoryUsage(VMA_MEMORY_USAGE_GPU_ONLY);
	
	m_tempImage.build(tempImageCreateInfo);
	
	m_tempImage.setNewAccessMask(VK_ACCESS_SHADER_READ_BIT);
	m_tempImage.setNewLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	m_queue->getCmdBuffer()->begin();
	m_queue->getCmdBuffer()->imageBarrier(&m_tempImage,
										  VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
	m_queue->getCmdBuffer()->end();
	
	m_tempImage.resetBarrier();
	
	m_queue->submit(m_queue->getCmdBuffer(), VK_NULL_HANDLE);
	
	m_queue->waitIdle();
	
	VkSamplerCreateInfo samplerCreateInfo;
	memset(&samplerCreateInfo, 0, sizeof(VkSamplerCreateInfo));
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.mipLodBias = 0;
	samplerCreateInfo.anisotropyEnable = VK_FALSE;
	samplerCreateInfo.maxAnisotropy = 0;
	samplerCreateInfo.minLod = 0;
	samplerCreateInfo.maxLod = 0;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	VK_CALL(m_device->vk.CreateSampler(device->handle(), &samplerCreateInfo, NULL, &m_sampler));
	
	VkDescriptorImageInfo descriptorImageInfo;
	memset(&descriptorImageInfo, 0, sizeof(VkDescriptorImageInfo));
	descriptorImageInfo.sampler = m_sampler;
	descriptorImageInfo.imageView = m_tempImage.view();
	descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkWriteDescriptorSet writeDescriptor;
	memset(&writeDescriptor, 0, sizeof(VkWriteDescriptorSet));
	writeDescriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptor.dstSet = m_descSet;
	writeDescriptor.dstBinding = 0;
	writeDescriptor.dstArrayElement = 0;
	writeDescriptor.descriptorCount = 1;
	writeDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptor.pImageInfo = &descriptorImageInfo;
	writeDescriptor.pBufferInfo = NULL;
	writeDescriptor.pTexelBufferView = NULL;

	m_device->vk.UpdateDescriptorSets(device->handle(), 1, &writeDescriptor, 0, NULL);
	
	VKLPipelineCreateInfo pipelineCreateInfo;
	pipelineCreateInfo.setShader(&m_shader).setRenderTarget(renderTarget);
	
	m_pipeline.build(pipelineCreateInfo);
	
	/*
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

	VKLShaderCreateInfo shaderCreateInfo;
	memset(&shaderCreateInfo, 0, sizeof(VKLShaderCreateInfo));
	shaderCreateInfo.shaderPaths = shaderPaths;
	shaderCreateInfo.shaderStages = stages;
	shaderCreateInfo.shaderCount = 2;
	shaderCreateInfo.bindings = bindings;
	shaderCreateInfo.bindingsCount = 2;
	
	
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
	 */
	
	updateProjection(renderTarget->getRenderArea().extent.width, renderTarget->getRenderArea().extent.height);
}

void Cursor::updateProjection(int width, int height) {
	m_screenSize[0] = (float)width;
	m_screenSize[1] = (float)height;
	/*
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

		width/2.0f, height/2.0f
	};
	 */
	//vklWriteToMemory(m_device, m_uniformBuffer->memory, uniformData, sizeof(float) * 18, 0);
}

void Cursor::render(VKLCommandBuffer* cmdBuffer) {
	VkDeviceSize vertexOffsets = 0;
	VkBuffer tempBuffHandle = m_vertBuffer.handle();
	
	m_device->vk.CmdBindVertexBuffers(cmdBuffer->handle(), 0, 1, &tempBuffHandle, &vertexOffsets);
	m_device->vk.CmdBindPipeline(cmdBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.handle());
	
	m_device->vk.CmdPushConstants(cmdBuffer->handle(), m_shader.getPipelineLayout(),
								  VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2, m_screenSize);
	
	m_device->vk.CmdBindDescriptorSets(cmdBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS,
									   m_shader.getPipelineLayout(), 0, 1, &m_descSet, 0, NULL);
	
	m_device->vk.CmdDraw(cmdBuffer->handle(), 12, 1, 0, 0);
}

void Cursor::destroy() {
	m_tempImage.destroy();
	
	m_device->vk.DestroySampler(m_device->handle(), m_sampler, m_device->allocationCallbacks());
	m_device->vk.DestroyDescriptorPool(m_device->handle(), m_pool, m_device->allocationCallbacks());
	
	m_pipeline.destroy();
	m_shader.destroy();
	m_vertBuffer.destroy();
}

