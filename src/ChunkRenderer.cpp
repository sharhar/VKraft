#include "ChunkRenderer.h"
#include "ChunkManager.h"
#include "Application.h"

#include <vector>
#include "lodepng.h"

ChunkRenderer::ChunkRenderer(Application* application) {
	m_device = &application->device;
	m_framebuffer = &application->framebuffer;
	m_application = application;
	
	m_chunkUniformBufferData = (ChunkUniform*) malloc(sizeof(ChunkUniform));
	memcpy(m_chunkUniformBufferData->proj, MathUtils::getPerspective(16.0f / 9.0f, 90), sizeof(float) * 16);
	
	int vertIDs[] = {
		0, 2, 1,
		2, 3, 1
	};
	
	m_vertBuffer.create(VKLBufferCreateInfo().device(m_device)
							.size(sizeof(int) * 6)
							.usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
							.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));
	
	m_vertBuffer.uploadData(application->transferQueue, vertIDs, sizeof(int) * 6, 0);
	
	size_t vertSize = 0;
	uint32_t* vertCode = (uint32_t*)FileUtils::readBinaryFile("res/cube-vert.spv", &vertSize);
	
	size_t fragSize = 0;
	uint32_t* fragCode = (uint32_t*)FileUtils::readBinaryFile("res/cube-frag.spv", &fragSize);
	
	m_shader.create(VKLShaderCreateInfo().device(&application->device)
							.addShaderModule(vertCode, vertSize, VK_SHADER_STAGE_VERTEX_BIT, "main")
							.addShaderModule(fragCode, fragSize, VK_SHADER_STAGE_FRAGMENT_BIT, "main")
							.addDescriptorSet()
								.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
							.end()
							.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * (16 * 2 + 3)));
	
	m_pipeline.create(VKLPipelineCreateInfo()
							.shader(&m_shader)
							.renderPass(&application->renderPass, 0)
							.vertexInput
								.addBinding(0, sizeof(int32_t))
									.addAttrib(0, VK_FORMAT_R32_SINT, 0)
								.end()
								.addBinding(1, sizeof(int32_t))
									.addAttrib(1, VK_FORMAT_R32_SINT, 0)
									.inputRate(VK_VERTEX_INPUT_RATE_INSTANCE)
								.end()
							.end());
	
	m_texture = new Texture(m_device, application->transferQueue, "res/pack.png", VK_FILTER_NEAREST);
	
	m_descriptorSet = new VKLDescriptorSet(&m_shader, 0);
	
	m_descriptorSet->writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_texture->view()->handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_texture->sampler());
}

void ChunkRenderer::rebuildPipeline() {
	//vklDestroyPipeline(m_device, m_pipeline);

	/*
	VkPushConstantRange push_constant;
	push_constant.offset = 0;
	push_constant.size = sizeof(float) * (16 * 2 + 3);
	push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	
	VKLGraphicsPipelineCreateInfo pipelineCreateInfo;
	memset(&pipelineCreateInfo, 0, sizeof(VKLGraphicsPipelineCreateInfo));
	pipelineCreateInfo.shader = m_shader;
	pipelineCreateInfo.renderPass = m_framebuffer->renderPass;
	pipelineCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	pipelineCreateInfo.extent.width = m_framebuffer->width;
	pipelineCreateInfo.extent.height = m_framebuffer->height;
	pipelineCreateInfo.pushConstantRanges = &push_constant;
	pipelineCreateInfo.pushConstantRangeCount = 1;
	
	vklCreateGraphicsPipeline(m_device, &m_pipeline, &pipelineCreateInfo);
	 */
}

void ChunkRenderer::render(const VKLCommandBuffer* cmdBuffer) {
	cmdBuffer->pushConstants(m_pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16 * 2, m_chunkUniformBufferData);
	
	cmdBuffer->bindPipeline(m_pipeline);
	cmdBuffer->bindVertexBuffer(m_vertBuffer, 0, 0);
	cmdBuffer->bindDescriptorSet(m_descriptorSet);
	
	for (int i = 0; i < m_application->chunkManager->getChunkCount(); i++) {
		Chunk* chunk = m_application->chunkManager->getChunkFromIndex(i);
		if (chunk->renderable()) {
			cmdBuffer->pushConstants(m_pipeline, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 16 * 2, sizeof(float) * 3, &chunk->renderPos);
			chunk->render(cmdBuffer);
		}
	}
	
	/*
	m_device->pvkCmdPushConstants(cmdBuffer, m_pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16 * 2, m_chunkUniformBufferData);

	VkViewport viewport = { 0, 0, m_framebuffer->width, m_framebuffer->height, 0, 1 };
	VkRect2D scissor = { {0, 0}, {m_framebuffer->width, m_framebuffer->height} };
	VkDeviceSize offsets = 0;

	m_device->pvkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
	m_device->pvkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
	m_device->pvkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->pipeline);
	m_device->pvkCmdBindVertexBuffers(cmdBuffer, 0, 1, &m_vertBuffer->buffer, &offsets);
	m_device->pvkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->pipelineLayout, 0, 1, &m_uniform->descriptorSet, 0, NULL);

	for (int i = 0; i < ChunkManager::getChunkCount(); i++) {
		Chunk* chunk = ChunkManager::getChunkFromIndex(i);
		if (chunk->renderable()) {
			m_device->pvkCmdPushConstants(cmdBuffer, m_pipeline->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 16 * 2, sizeof(float) * 3, &chunk->renderPos);
			chunk->render(cmdBuffer);
		}
	}
	 */
}

ChunkUniform* ChunkRenderer::getUniform() {
	return m_chunkUniformBufferData;
}

void ChunkRenderer::destroy() {
	m_descriptorSet->destroy();
	delete m_descriptorSet;
	
	m_texture->destroy();
	delete m_texture;
	
	m_pipeline.destroy();
	m_shader.destroy();
	m_vertBuffer.destroy();
	
	free(m_chunkUniformBufferData);
}
