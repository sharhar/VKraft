#include "Application.h"
#include "ChunkManager.h"

ChunkManager::ChunkManager(Application* application) {
	m_application = application;
	
	m_chunks = new Chunk[16*16*16];
	m_chunkCount = 0;
	
	size_t shaderSize = 0;
	uint32_t* shaderCode = (uint32_t*)FileUtils::readBinaryFile("res/world-gen.spv", &shaderSize);
	
	struct PC {
		float x, y, z;
		int seed;
	};
	
	m_layout.create(VKLPipelineLayoutCreateInfo()
							.device(&application->device)
							.addShaderModule(shaderCode, shaderSize, VK_SHADER_STAGE_COMPUTE_BIT, "main")
							.addDescriptorSet()
								.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
							.end()
							.addPushConstant(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(struct PC)));
	
	m_computePipeline.create(VKLPipelineCreateInfo().layout(&m_layout));
	
	VKLBuffer testBuffer(VKLBufferCreateInfo()
								.size(sizeof(float) * 5)
								.device(&application->device)
								.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
								.memoryUsage(VMA_MEMORY_USAGE_GPU_TO_CPU));
	
	VKLDescriptorSet descSet(&m_layout, 0);
	
	descSet.writeBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &testBuffer, 0, sizeof(float) * 5);
	
	struct PC pc;
	pc.x = 69;
	pc.y = 2;
	pc.z = 3;
	
	pc.seed = 7;
	
	VKLCommandBuffer cmdBuff(application->computeQueue);
	
	cmdBuff.begin();
	
	cmdBuff.bindPipeline(m_computePipeline);
	cmdBuff.bindDescriptorSet(descSet);
	cmdBuff.pushConstants(m_computePipeline, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(struct PC), &pc);
	
	cmdBuff.dispatch(5, 1, 1);
	
	cmdBuff.end();
	
	application->computeQueue->submit(&cmdBuff, VK_NULL_HANDLE);
	application->computeQueue->waitIdle();
	
	float resDat[5];
	
	for(int i = 0; i < 5; i++) {
		printf("%d = %g\n", i, resDat[i]);
	}
	
	testBuffer.getData(resDat, sizeof(float) * 5, 0);
	
	for(int i = 0; i < 5; i++) {
		printf("%d = %g\n", i, resDat[i]);
	}
	
	descSet.destroy();
	cmdBuff.destroy();
	testBuffer.destroy();
	
}

uint32_t ChunkManager::getChunkCount() {
	return m_chunkCount;
}

void ChunkManager::addChunk(MathUtils::Vec3i pos) {
	m_chunkCount++;
	m_chunks[m_chunkCount-1].init(this, pos);
	m_chunks[m_chunkCount-1].updateNearChunks();
}

int ChunkManager::getChunkAt(MathUtils::Vec3i pos) {
	for(int i = 0; i < m_chunkCount; i++) {
		if(m_chunks[i].atPos(pos)) {
			return i;
		}
	}
	
	return -1;
}

Chunk* ChunkManager::getChunkFromIndex(int index) {
	if(index == -1) {
		return NULL;
	}
	
	return &(m_chunks[index]);
}

void ChunkManager::destroy() {
	for (int i = 0; i < m_chunkCount; i++) {
		m_chunks[i].destroy();
	}

	delete[] m_chunks;
	
	m_computePipeline.destroy();
	m_layout.destroy();
}
