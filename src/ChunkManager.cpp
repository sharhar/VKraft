//
//  ChunkManager.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#include "ChunkManager.h"

#include <vector>
#include "lodepng.h"

std::thread* ChunkManager::m_thread = NULL;
ChunkUniform* ChunkManager::m_chunkUniformBufferData = NULL;
VKLDevice* ChunkManager::m_device = NULL;
VKLFrameBuffer* ChunkManager::m_framebuffer = NULL;
VKLBuffer* ChunkManager::m_uniformBuffer = NULL;
VKLShader* ChunkManager::m_shader = NULL;
VKLUniformObject* ChunkManager::m_uniform = NULL;
VKLPipeline* ChunkManager::m_pipeline = NULL;
VKLTexture* ChunkManager::m_texture = NULL;

void ChunkManager::init(VKLDevice* device, int width, int height) {
	m_device = device;
	
	m_chunkUniformBufferData = (ChunkUniform*) malloc(sizeof(ChunkUniform));
	memcpy(m_chunkUniformBufferData->proj, getPerspective(16.0f / 9.0f, 90), sizeof(float) * 16);
	
	vklCreateFrameBuffer(device->deviceGraphicsContexts[0], &m_framebuffer, width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	vklCreateBuffer(device, &m_uniformBuffer, VK_FALSE, sizeof(ChunkUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	
	char* shaderPaths[2];
	shaderPaths[0] = "res/cube-vert.spv";
	shaderPaths[1] = "res/cube-frag.spv";

	VkShaderStageFlagBits stages[2];
	stages[0] = VK_SHADER_STAGE_VERTEX_BIT;
	stages[1] = VK_SHADER_STAGE_FRAGMENT_BIT;

	size_t offsets[2] = { 0, sizeof(float) * 4 };
	VkFormat formats[2] = { VK_FORMAT_R32G32B32A32_SFLOAT, VK_FORMAT_R32G32_SFLOAT };

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
	
	uint32_t vertexInputAttributeBindings[] = {0, 0};

	VKLShaderCreateInfo shaderCreateInfo;
	memset(&shaderCreateInfo, 0, sizeof(VKLShaderCreateInfo));
	shaderCreateInfo.shaderPaths = shaderPaths;
	shaderCreateInfo.shaderStages = stages;
	shaderCreateInfo.shaderCount = 2;
	shaderCreateInfo.bindings = bindings;
	shaderCreateInfo.bindingsCount = 2;
	shaderCreateInfo.vertexInputAttributesCount = 2;
	shaderCreateInfo.vertexInputAttributeOffsets = offsets;
	shaderCreateInfo.vertexInputAttributeFormats = formats;
	shaderCreateInfo.vertexInputAttributeBindings = vertexInputAttributeBindings;
	
	VkVertexInputRate vertInputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	size_t stride = sizeof(float) * 6;
	shaderCreateInfo.vertexBindingsCount = 1;
	shaderCreateInfo.vertexBindingInputRates = &vertInputRate;
	shaderCreateInfo.vertexBindingStrides = &stride;
	
	vklCreateShader(device, &m_shader, &shaderCreateInfo);
	
	vklCreateUniformObject(device, &m_uniform, m_shader);
	
	vklSetUniformBuffer(device, m_uniform, m_uniformBuffer, 0);
	
	VKLGraphicsPipelineCreateInfo pipelineCreateInfo;
	memset(&pipelineCreateInfo, 0, sizeof(VKLGraphicsPipelineCreateInfo));
	pipelineCreateInfo.shader = m_shader;
	pipelineCreateInfo.renderPass = m_framebuffer->renderPass;
	pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineCreateInfo.extent.width = m_framebuffer->width;
	pipelineCreateInfo.extent.height = m_framebuffer->height;
	
	vklCreateGraphicsPipeline(device, &m_pipeline, &pipelineCreateInfo);
	
	uint32_t tex_width, tex_height;
	
	std::vector<unsigned char> imageData;
	
	unsigned int error = lodepng::decode(imageData, tex_width, tex_height, "res/pack.png");
	
	if (error != 0) {
		std::cout << "Error loading image: " << error << "\n";
		return;
	}
	
	VKLTextureCreateInfo textureCreateInfo;
	textureCreateInfo.width = tex_width;
	textureCreateInfo.height = tex_height;
	textureCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	textureCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
	textureCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
	textureCreateInfo.filter = VK_FILTER_NEAREST;
	textureCreateInfo.colorSize = sizeof(char);
	textureCreateInfo.colorCount = 4;
	
	vklCreateStagedTexture(device->deviceGraphicsContexts[0], &m_texture, &textureCreateInfo, imageData.data());
	
	vklSetUniformTexture(device, m_uniform, m_texture, 1);
	
	m_thread = new std::thread(ChunkManager::internalThreadFunction);
}

void ChunkManager::render(VkCommandBuffer cmdBuffer) {
	vklBeginRender(m_device, m_framebuffer, cmdBuffer);
	
	
	
	vklEndRender(m_device, m_framebuffer, cmdBuffer);
}

VKLFrameBuffer* ChunkManager::getFramebuffer() {
	return m_framebuffer;
}

void ChunkManager::destroy() {
	m_thread->join();
	
	vklDestroyTexture(m_device, m_texture);
	vklDestroyPipeline(m_device, m_pipeline);
	vklDestroyUniformObject(m_device, m_uniform);
	vklDestroyShader(m_device, m_shader);
	vklDestroyBuffer(m_device, m_uniformBuffer);
	vklDestroyFrameBuffer(m_device, m_framebuffer);
	free(m_chunkUniformBufferData);
}

void ChunkManager::internalThreadFunction() {
	printf("Chunk thread!\n");
}
