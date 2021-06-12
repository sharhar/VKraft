//
//  FontEngine.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#include "FontEngine.h"

#include "lodepng.h"
#include <iostream>

VKLBuffer* FontEngine::m_uniformBuffer = NULL;
VKLDevice* FontEngine::m_device = NULL;
VKLShader* FontEngine::m_shader = NULL;
VKLUniformObject* FontEngine::m_uniform = NULL;
VKLTexture* FontEngine::m_texture = NULL;
VKLPipeline* FontEngine::m_pipeline = NULL;

void FontEngine::init(VKLDevice *device, VKLSwapChain *swapChain) {
	m_device = device;
	
	vklCreateBuffer(device, &m_uniformBuffer, VK_FALSE, sizeof(float) * 16, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	
	char* shaderPaths[2];
	shaderPaths[0] = "res/font-vert.spv";
	shaderPaths[1] = "res/font-frag.spv";

	VkShaderStageFlagBits stages[2];
	stages[0] = VK_SHADER_STAGE_VERTEX_BIT;
	stages[1] = VK_SHADER_STAGE_FRAGMENT_BIT;

	size_t offsets[2] = { 0, sizeof(float) * 2 };
	VkFormat formats[2] = { VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32_SFLOAT };

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
	shaderCreateInfo.vertexInputAttributeStride = sizeof(float) * 4;
	shaderCreateInfo.vertexInputAttributesCount = 2;
	shaderCreateInfo.vertexInputAttributeOffsets = offsets;
	shaderCreateInfo.vertexInputAttributeFormats = formats;
	
	vklCreateShader(device, &m_shader, &shaderCreateInfo);
	
	VKLGraphicsPipelineCreateInfo pipelineCreateInfo;
	memset(&pipelineCreateInfo, 0, sizeof(VKLGraphicsPipelineCreateInfo));
	pipelineCreateInfo.shader = m_shader;
	pipelineCreateInfo.renderPass = swapChain->backBuffer->renderPass;
	pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfo.cullMode = VK_CULL_MODE_NONE;
	pipelineCreateInfo.extent.width = swapChain->width;
	pipelineCreateInfo.extent.height = swapChain->height;

	vklCreateGraphicsPipeline(device, &m_pipeline, &pipelineCreateInfo);
	
	vklCreateUniformObject(device, &m_uniform, m_shader);
	
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
	
	vklSetUniformBuffer(m_device, m_uniform, m_uniformBuffer, 0);
	vklSetUniformTexture(m_device, m_uniform, m_texture, 1);
	
	updateProjection(swapChain->width, swapChain->height);
}

void FontEngine::updateProjection(int width, int height) {
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
		-(r + l) / (r - l), -(t + b) / (t - b), -(f + n) / (f - n), 1
	};
	
	vklWriteToMemory(m_device, m_uniformBuffer->memory, uniformData, sizeof(float) * 16);
}

void FontEngine::destroy() {
	vklDestroyPipeline(m_device, m_pipeline);
	vklDestroyUniformObject(m_device, m_uniform);
	vklDestroyTexture(m_device, m_texture);
	vklDestroyShader(m_device, m_shader);
}
