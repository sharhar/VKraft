//
//  FontEngine.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#include "TextObject.h"

#include "lodepng.h"
#include <iostream>

#include "Utils.h"

const VKLDevice* TextObject::m_device = NULL;
const VKLRenderPass* TextObject::m_renderPass = NULL;

VKLBuffer TextObject::m_vertBuffer;
VKLShader TextObject::m_shader;
VKLPipeline TextObject::m_pipeline;

void TextObject::init(const VKLDevice *device, const VKLQueue* transferQueue, const VKLRenderPass* renderPass) {
	m_device = device;
	m_renderPass = renderPass;
	
	float verts[24] = {
		0, 0, 0, 0,
		0, 1, 0, 1,
		1, 0, 1, 0,
		
		0, 1, 0, 1,
		1, 0, 1, 0,
		1, 1, 1, 1
	};

	m_vertBuffer.create(VKLBufferCreateInfo()
									.device(m_device).size(6 * 4 * sizeof(float))
									.usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
									.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));

	m_vertBuffer.uploadData(transferQueue, verts, 6 * 4 * sizeof(float), 0);

	size_t vertSize = 0;
	uint32_t* vertCode = (uint32_t*)readBinaryFile("res/font-vert.spv", &vertSize);

	size_t fragSize = 0;
	uint32_t* fragCode = (uint32_t*)readBinaryFile("res/font-frag.spv", &fragSize);

	/*
	m_shader.create(VKLShaderCreateInfo()
						.device(m_device)
						.addShaderModule(vertCode, vertSize, VK_SHADER_STAGE_VERTEX_BIT, "main")
						.addShaderModule(fragCode, fragSize, VK_SHADER_STAGE_FRAGMENT_BIT, "main")
						.addVertexInputBinding(0)
							.setStride(sizeof(float) * 4)
							.addAttrib(0, VK_FORMAT_R32G32_SFLOAT, 0)
							.addAttrib(1, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 2)
						.end()
						.addVertexInputBinding(1)
							.setStride(sizeof(int32_t))
							.setInputRate(VK_VERTEX_INPUT_RATE_INSTANCE)
							.addAttrib(2, VK_FORMAT_R32_SINT, 0)
						.end()
						.addDescriptorSet()
							.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT)
							.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
						.end());

	m_pipeline.create(VKLPipelineCreateInfo().setShader(&m_shader).setRenderTarget(renderTarget));
	*/

	/*
	
	VKLGraphicsPipelineCreateInfo pipelineCreateInfo;
	memset(&pipelineCreateInfo, 0, sizeof(VKLGraphicsPipelineCreateInfo));
	pipelineCreateInfo.shader = m_shader;
	pipelineCreateInfo.renderPass = framebuffer->renderPass;
	pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfo.cullMode = VK_CULL_MODE_NONE;
	pipelineCreateInfo.extent.width = framebuffer->width;
	pipelineCreateInfo.extent.height = framebuffer->height;

	vklCreateGraphicsPipeline(device, &m_pipeline, &pipelineCreateInfo);

	uint32_t tex_width, tex_height;
	
	std::vector<unsigned char> imageData;
	
	unsigned int error = lodepng::decode(imageData, tex_width, tex_height, "res/font.png");
	
	if (error != 0) {
		std::cout << "Error loading image: " << error << "\n";
	}
	
	VKLTextureCreateInfo textureCreateInfo;
	textureCreateInfo.width = tex_width;
	textureCreateInfo.height = tex_height;
	textureCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	textureCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	textureCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	textureCreateInfo.filter = VK_FILTER_LINEAR;
	textureCreateInfo.colorSize = sizeof(char);
	textureCreateInfo.colorCount = 4;
	
	vklCreateStagedTexture(m_device->deviceGraphicsContexts[0], &m_texture, &textureCreateInfo, imageData.data());
	*/
}

void TextObject::updateProjection() {
	//VkRect2D area = m_renderTarget->getRenderArea();

	float r = 800;//area.extent.width;
	float l = 0;
	float t = 600;//area.extent.height;
	float b = 0;
	float f = 1;
	float n = -1;

	float uniformData[] = {
		2 / (r - l), 0, 0, 0,
		0, 2 / (t - b), 0, 0,
		0, 0, -2 / (f - n), 0,
		-(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1
	};
	
	//vklWriteToMemory(m_device, m_uniformBuffer->memory, uniformData, sizeof(float) * 16, 0);
}

void TextObject::destroy() {
	m_pipeline.destroy();
	m_shader.destroy();
	m_vertBuffer.destroy();
}

TextObject::TextObject(int maxCharNum) {
	/*
	vklCreateBuffer(m_device, &m_uniformBuffer, VK_FALSE, sizeof(float) * 19, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	vklCreateBuffer(m_device, &m_instanceBuffer, VK_FALSE, sizeof(int) * maxCharNum, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	
	vklCreateUniformObject(m_device, &m_uniform, m_shader);
	
	vklSetUniformBuffer(m_device, m_uniform, m_uniformBuffer, 0);
	vklSetUniformTexture(m_device, m_uniform, m_texture, 1);
	*/
	updateProjection();
}

void TextObject::render(VkCommandBuffer cmdBuffer) {
	/*
	VkDeviceSize offsets = 0;
	m_device->pvkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->pipeline);
	m_device->pvkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vertBuffer->buffer, &offsets);
	m_device->pvkCmdBindVertexBuffers(cmdBuffer, 1, 1, &m_instanceBuffer->buffer, &offsets);
	m_device->pvkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->pipelineLayout, 0, 1, &m_uniform->descriptorSet, 0, NULL);
	
	m_device->pvkCmdDraw(cmdBuffer, 6, m_charNum, 0, 0);
	*/
}

void TextObject::setText(const std::string& str) {
	int* ids = new int[str.length()];
	
	for(int i = 0; i < str.length(); i++) {
		ids[i] = (int) str[i];
	}
	
	//vklWriteToMemory(m_device, m_instanceBuffer->memory, ids, sizeof(int) * str.length(), 0);
	
	delete[] ids;
	
	m_charNum = str.length();
}

void TextObject::setCoords(float xPos, float yPos, float size) {
	float buff[3] = {xPos, yPos, size};
	//vklWriteToMemory(m_device, m_uniformBuffer->memory, buff, sizeof(float) * 3, sizeof(float) * 16);
}
