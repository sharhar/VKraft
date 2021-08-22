//
//  Cursor.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#include "Cursor.h"
#include "Application.h"
#include "Utils.h"

Cursor::Cursor(Application* application) {
	m_application = application;
	
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
	vertBufferCreateInfo.device(&m_application->device).size(12 * 2 * sizeof(float))
						.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY)
						.usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
	
	m_vertBuffer.create(vertBufferCreateInfo);
	m_vertBuffer.uploadData(m_application->transferQueue, cursorVerts, 12 * 2 * sizeof(float), 0);
	
	size_t vertSize = 0;
	uint32_t* vertCode = (uint32_t*)readBinaryFile("res/cursor-vert.spv", &vertSize);
	
	size_t fragSize = 0;
	uint32_t* fragCode = (uint32_t*)readBinaryFile("res/cursor-frag.spv", &fragSize);
	
	m_shader.create(VKLShaderCreateInfo().device(&m_application->device)
							.addShaderModule(vertCode, vertSize, VK_SHADER_STAGE_VERTEX_BIT, "main")
							.addShaderModule(fragCode, fragSize, VK_SHADER_STAGE_FRAGMENT_BIT, "main")
							.addDescriptorSet()
								.addBinding(0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
							.end()
							.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2));

	m_pipeline.create(VKLPipelineCreateInfo()
							.shader(&m_shader)
							.renderPass(&m_application->renderPass, 1)
							.vertexInput
								.addBinding(0, sizeof(float) * 2)
									.addAttrib(0, VK_FORMAT_R32G32_SFLOAT, 0)
								.end()
							.end());
	
	m_descriptorSet = new VKLDescriptorSet(&m_shader, 0);
	
	m_descriptorSet->writeImage(0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_application->backBufferViews[1].handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_NULL_HANDLE);

	/*
	
	vklCreateUniformObject(device, &m_uniform, m_shader);
	
	vklCreateBuffer(device, &m_uniformBuffer, VK_FALSE, sizeof(float) * 18, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	
	vklSetUniformBuffer(m_device, m_uniform, m_uniformBuffer, 0);
	vklSetUniformFramebuffer(m_device, m_uniform, framebuffer, 1);
	
	updateProjection(swapChain->width, swapChain->height);
	 */
	
	updateProjection(800, 600);//renderTarget->getRenderArea().extent.width, renderTarget->getRenderArea().extent.height);
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
	VkDescriptorSet descSet = m_descriptorSet->handle();
	
	m_application->device.vk.CmdBindVertexBuffers(cmdBuffer->handle(), 0, 1, &tempBuffHandle, &vertexOffsets);
	m_application->device.vk.CmdBindPipeline(cmdBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline.handle());
	
	m_application->device.vk.CmdPushConstants(cmdBuffer->handle(), m_shader.getPipelineLayout(),
								  VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 2, m_screenSize);
	
	m_application->device.vk.CmdBindDescriptorSets(cmdBuffer->handle(), VK_PIPELINE_BIND_POINT_GRAPHICS, m_shader.getPipelineLayout(), 0, 1, &descSet, 0, NULL);
	
	m_application->device.vk.CmdDraw(cmdBuffer->handle(), 12, 1, 0, 0);
}

void Cursor::destroy() {
	m_descriptorSet->destroy();
	
	delete m_descriptorSet;
	
	m_pipeline.destroy();
	m_shader.destroy();
	m_vertBuffer.destroy();
}

