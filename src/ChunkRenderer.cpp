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
	
	m_layout.create(VKLPipelineLayoutCreateInfo().device(&application->device)
							.addShaderModule(vertCode, vertSize, VK_SHADER_STAGE_VERTEX_BIT, "main")
							.addShaderModule(fragCode, fragSize, VK_SHADER_STAGE_FRAGMENT_BIT, "main")
							.addDescriptorSet()
								.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
								//.addBinding(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT)
							.end()
							.addPushConstant(VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * (16 * 2)));
	
	m_pipeline.create(VKLPipelineCreateInfo()
							.layout(&m_layout)
							.renderPass(&application->renderPass, 0)
							.vertexInput
								.addBinding(0, sizeof(int32_t))
									.addAttrib(0, VK_FORMAT_R32_SINT, 0)
								.end()
								.addBinding(1, sizeof(int32_t))
									.addAttrib(1, VK_FORMAT_R32_SINT, 0)
									.inputRate(VK_VERTEX_INPUT_RATE_INSTANCE)
								.end()
								.addBinding(2, sizeof(int32_t) * 3)
									.addAttrib(2, VK_FORMAT_R32G32B32_SINT, 0)
									.inputRate(VK_VERTEX_INPUT_RATE_INSTANCE)
								.end()
							.end());
	
	m_texture = new Texture(m_device, application->transferQueue, "res/pack.png", VK_FILTER_NEAREST);
	
	m_descriptorSet = new VKLDescriptorSet(&m_layout, 0);
	
	m_descriptorSet->writeImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_texture->view()->handle(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, m_texture->sampler());
	//m_descriptorSet->writeBuffer(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &m_application->chunkManager->m_positionBuffer, 0, sizeof(uint32_t) * 512 * 3);
}

void ChunkRenderer::rebuildPipeline() {
	
}

void ChunkRenderer::render(const VKLCommandBuffer* cmdBuffer) {
	cmdBuffer->pushConstants(m_pipeline, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float) * 16 * 2, m_chunkUniformBufferData);
	
	cmdBuffer->bindPipeline(m_pipeline);
	cmdBuffer->bindDescriptorSet(m_descriptorSet);
	
	cmdBuffer->bindVertexBuffer(m_vertBuffer, 0, 0);
	cmdBuffer->bindVertexBuffer(m_application->chunkManager->m_facesBuffer, 1, 0);
	cmdBuffer->bindVertexBuffer(m_application->chunkManager->m_positionBuffer, 2, 0);
	
	cmdBuffer->draw(6, m_application->chunkManager->m_faceNum, 0, 0);
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
	m_layout.destroy();
	m_vertBuffer.destroy();
	
	free(m_chunkUniformBufferData);
}
