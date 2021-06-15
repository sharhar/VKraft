//
//  ChunkRenderer.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#include "ChunkRenderer.h"

#include <vector>
#include "lodepng.h"

std::vector<Chunk> ChunkRenderer::m_chunks = std::vector<Chunk>();
ChunkUniform* ChunkRenderer::m_chunkUniformBufferData = NULL;
VKLDevice* ChunkRenderer::m_device = NULL;
VKLFrameBuffer* ChunkRenderer::m_framebuffer = NULL;
VKLBuffer* ChunkRenderer::m_uniformBuffer = NULL;
VKLBuffer* ChunkRenderer::m_vertBuffer = NULL;
VKLShader* ChunkRenderer::m_shader = NULL;
VKLUniformObject* ChunkRenderer::m_uniform = NULL;
VKLPipeline* ChunkRenderer::m_pipeline = NULL;
VKLTexture* ChunkRenderer::m_texture = NULL;

static int calcVertInt(float posx, float posy, float posz, float u, float v, int face) {
	int result = 0;
	
	if (posx > 0) {
		result = result | 1;
		
	}
	
	if (posy > 0) {
		result = result | 2;
		
	}
	
	if (posz > 0) {
		result = result | 4;
		
	}
	
	if (u > 0.5) {
		result = result | 8;
		
	}
	
	if (v > 0.5) {
		result = result | 16;
		
	}
	
	result = result | ((face & 0b111111) << 5);
	
	return result;
	
}

void ChunkRenderer::init(VKLDevice* device, VKLFrameBuffer* framebuffer) {
	m_device = device;
	m_framebuffer = framebuffer;
	
	m_chunkUniformBufferData = (ChunkUniform*) malloc(sizeof(ChunkUniform));
	memcpy(m_chunkUniformBufferData->proj, getPerspective(16.0f / 9.0f, 90), sizeof(float) * 16);
	
	int cubeVertis[] = {
		// back face
		calcVertInt(-0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1),
		calcVertInt(-0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1),
		calcVertInt( 0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1),
		
		calcVertInt( 0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1),
		calcVertInt(-0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 1),
		calcVertInt( 0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 1),
		
		// front face
	    calcVertInt( 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 2),
	    calcVertInt(-0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 2),
	    calcVertInt(-0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 2),
	   
		calcVertInt( 0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 2),
	    calcVertInt(-0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 2),
		calcVertInt( 0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 2),
		
		// left face
		calcVertInt(-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 4),
		calcVertInt(-0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 4),
		calcVertInt(-0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 4),
		 
		calcVertInt(-0.5f, -0.5f,  0.5f, 1.0f, 0.0f, 4),
		calcVertInt(-0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 4),
		calcVertInt(-0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 4),
		 
		// right face
		calcVertInt( 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 8),
		calcVertInt( 0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 8),
		calcVertInt( 0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 8),

		calcVertInt( 0.5f,  0.5f,  0.5f, 0.0f, 1.0f, 8),
		calcVertInt( 0.5f, -0.5f,  0.5f, 0.0f, 0.0f, 8),
		calcVertInt( 0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 8),

		 // top face
		calcVertInt(  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 16),
		calcVertInt( -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 16),
		calcVertInt( -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 16),
		
		calcVertInt(  0.5f, -0.5f,  0.5f, 1.0f, 1.0f, 16),
		calcVertInt( -0.5f, -0.5f,  0.5f, 0.0f, 1.0f, 16),
		calcVertInt(  0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 16),
		
		// back face
		calcVertInt(  0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 32),
		calcVertInt( -0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 32),
		calcVertInt( -0.5f,  0.5f,  0.5f, 0.0f, 0.0f, 32),
		
		calcVertInt( 0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 32),
		calcVertInt(-0.5f,  0.5f, -0.5f, 0.0f, 1.0f, 32),
		calcVertInt( 0.5f,  0.5f,  0.5f, 1.0f, 0.0f, 32),
	};
	
	vklCreateStagedBuffer(m_device->deviceGraphicsContexts[0], &m_vertBuffer, cubeVertis, sizeof(int) * 6 * 6, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	
	vklCreateBuffer(device, &m_uniformBuffer, VK_FALSE, sizeof(ChunkUniform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);
	
	char* shaderPaths[2];
	shaderPaths[0] = "res/cube-vert.spv";
	shaderPaths[1] = "res/cube-frag.spv";

	VkShaderStageFlagBits stages[2];
	stages[0] = VK_SHADER_STAGE_VERTEX_BIT;
	stages[1] = VK_SHADER_STAGE_FRAGMENT_BIT;

	size_t offsets[] = { 0, 0 };
	VkFormat formats[] = { VK_FORMAT_R32_SINT, VK_FORMAT_R32_SINT };

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
	
	uint32_t vertexInputAttributeBindings[] = {0, 1};

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
	
	VkVertexInputRate vertInputRate[] = {VK_VERTEX_INPUT_RATE_VERTEX, VK_VERTEX_INPUT_RATE_INSTANCE};
	size_t stride[] = {sizeof(int), sizeof(int)};
	shaderCreateInfo.vertexBindingsCount = 2;
	shaderCreateInfo.vertexBindingInputRates = vertInputRate;
	shaderCreateInfo.vertexBindingStrides = stride;
	
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
}

void ChunkRenderer::rebuildPipeline() {
	vklDestroyPipeline(m_device, m_pipeline);
	
	VKLGraphicsPipelineCreateInfo pipelineCreateInfo;
	memset(&pipelineCreateInfo, 0, sizeof(VKLGraphicsPipelineCreateInfo));
	pipelineCreateInfo.shader = m_shader;
	pipelineCreateInfo.renderPass = m_framebuffer->renderPass;
	pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineCreateInfo.extent.width = m_framebuffer->width;
	pipelineCreateInfo.extent.height = m_framebuffer->height;
	
	vklCreateGraphicsPipeline(m_device, &m_pipeline, &pipelineCreateInfo);
}

void ChunkRenderer::render(VkCommandBuffer cmdBuffer) {
	vklWriteToMemory(m_device, m_uniformBuffer->memory, m_chunkUniformBufferData, sizeof(ChunkUniform), 0);
	
	VkViewport viewport = { 0, 0, m_framebuffer->width, m_framebuffer->height, 0, 1 };
	VkRect2D scissor = { {0, 0}, {m_framebuffer->width, m_framebuffer->height} };
	VkDeviceSize offsets = 0;
	
	m_device->pvkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	m_device->pvkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
	m_device->pvkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->pipeline);
	m_device->pvkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vertBuffer->buffer, &offsets);
	m_device->pvkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->pipelineLayout, 0, 1, &m_uniform->descriptorSet, 0, NULL);
	
	for (Chunk chunk: m_chunks) {
		chunk.render(cmdBuffer);
	}
}

ChunkUniform* ChunkRenderer::getUniform() {
	return m_chunkUniformBufferData;
}

void ChunkRenderer::destroy() {
	vklDestroyTexture(m_device, m_texture);
	vklDestroyPipeline(m_device, m_pipeline);
	vklDestroyUniformObject(m_device, m_uniform);
	vklDestroyShader(m_device, m_shader);
	vklDestroyBuffer(m_device, m_uniformBuffer);
	vklDestroyBuffer(m_device, m_vertBuffer);
	vklDestroyFrameBuffer(m_device, m_framebuffer);
	free(m_chunkUniformBufferData);
}

void ChunkRenderer::addChunk(Vec3i pos) {
	m_chunks.push_back(Chunk(m_device, pos));
}
