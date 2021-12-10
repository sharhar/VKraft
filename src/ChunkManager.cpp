#include "Application.h"
#include "ChunkManager.h"
#include "Utils.h"

#define BATCH_SIZE 32

typedef struct CubeNoise {
	float nz1;
	float nz2;

	float nz;

	float nzc;

	float cnzc;
	float inzc;
} CubeNoise;

ChunkManager::ChunkManager(Application* application) : m_cmdBuffer(application->computeQueue), m_transferCmdBuffer(application->transferQueue){
	m_application = application;
	
	m_chunks = new Chunk[16*16*16];
	m_chunkCount = 0;
	
	size_t shaderSize = 0;
	uint32_t* shaderCode = (uint32_t*)FileUtils::readBinaryFile("res/world-gen.spv", &shaderSize);
	
	m_terrainLayout.create(VKLPipelineLayoutCreateInfo()
							.device(&application->device)
							.addShaderModule(shaderCode, shaderSize, VK_SHADER_STAGE_COMPUTE_BIT, "main")
							.addDescriptorSet()
								.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
								.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
								.addBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
							.end());
	
	free(shaderCode);
	
	shaderCode = (uint32_t*)FileUtils::readBinaryFile("res/heightmap.spv", &shaderSize);
	
	m_heightmapLayout.create(VKLPipelineLayoutCreateInfo()
							.device(&application->device)
							.addShaderModule(shaderCode, shaderSize, VK_SHADER_STAGE_COMPUTE_BIT, "main")
							.addDescriptorSet()
								.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
								.addBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
							.end());
	
	free(shaderCode);
	
	m_terrainPipeline.create(VKLPipelineCreateInfo().layout(&m_terrainLayout));
	m_heightmapPipeline.create(VKLPipelineCreateInfo().layout(&m_heightmapLayout));
	
	m_seed = 1337;
	
	m_cubeNoises = new CubeNoise[16 * 16 * 16];
	
	m_resultBuffer.create(VKLBufferCreateInfo()
						  .size(sizeof(uint8_t) * 16 * 16 * 16 * BATCH_SIZE)
								.device(&application->device)
								.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
								.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));
	
	m_heightmapBuffer.create(VKLBufferCreateInfo()
							 .size(sizeof(int32_t) * 16 * 16 * BATCH_SIZE)
								.device(&application->device)
								.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
								.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));
	
	m_chunkInfoBuffer.create(VKLBufferCreateInfo()
							 .size(sizeof(int) * (BATCH_SIZE * 3 + 1 + BATCH_SIZE * 2))
								   .device(&application->device)
								   .usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
								   .memoryUsage(VMA_MEMORY_USAGE_CPU_TO_GPU));
	
	m_stagingBuffer.create(VKLBufferCreateInfo()
						  .size(sizeof(uint8_t) * 16 * 16 * 16 * BATCH_SIZE)
								.device(&application->device)
								.usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT)
								.memoryUsage(VMA_MEMORY_USAGE_GPU_TO_CPU));
	
	m_facesBuffer.create(VKLBufferCreateInfo()
				   .device(&application->device)
				   .size(sizeof(uint32_t) * 16 * 16 * 16 * 3 * 512)
				   .usage(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT)
				   .memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));
	
	m_facesStagingBuffer.create(VKLBufferCreateInfo()
								.device(&application->device)
								.size(sizeof(uint32_t) * 16 * 16 * 16 * 3)
								.usage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
								.memoryUsage(VMA_MEMORY_USAGE_CPU_TO_GPU));
	
	m_positionBuffer.create(VKLBufferCreateInfo()
								 .device(&application->device)
								 .size(sizeof(int32_t) * 16 * 16 * 16 * 3 * 512 * 3)
								 .usage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
								 .memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));
	
	m_faceNum = 0;
	
	m_terrainDescSet = new VKLDescriptorSet(&m_terrainLayout, 0);
	
	m_terrainDescSet->writeBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &m_resultBuffer, 0, sizeof(uint8_t) * 16 * 16 * 16 * BATCH_SIZE);
	m_terrainDescSet->writeBuffer(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &m_chunkInfoBuffer, 0, sizeof(int32_t) * (BATCH_SIZE * 3 + 1));
	m_terrainDescSet->writeBuffer(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &m_heightmapBuffer, 0, sizeof(int32_t) * 16 * 16 * BATCH_SIZE);
	
	m_heightmapDescSet = new VKLDescriptorSet(&m_heightmapLayout, 0);
	
	m_heightmapDescSet->writeBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &m_heightmapBuffer, 0, sizeof(int32_t) * 16 * 16 * BATCH_SIZE);
	m_heightmapDescSet->writeBuffer(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &m_chunkInfoBuffer, 0, sizeof(int32_t) * (BATCH_SIZE * 3 + 1));
}

uint32_t ChunkManager::getChunkCount() {
	return m_chunkCount;
}

void ChunkManager::queueChunk(MathUtils::Vec3i pos) {
	m_queue.push_back(pos);
}

void ChunkManager::processBatch(int startIndex, int batchSize, Timer& tm) {
	int32_t* chunkInfo = (int32_t*)m_chunkInfoBuffer.map();
	
	uint32_t heightmapChunkCount = 0;
	
	for(int i = startIndex; i < startIndex + batchSize; i++) {
		chunkInfo[(i - startIndex)*3 + 0] = m_queue[i].x;
		chunkInfo[(i - startIndex)*3 + 1] = -m_queue[i].y;
		chunkInfo[(i - startIndex)*3 + 2] = m_queue[i].z;
		
		uint8_t foundHeightmapCoord = 0;
		
		if(!foundHeightmapCoord) {
			chunkInfo[BATCH_SIZE*3+heightmapChunkCount*2+1] = m_queue[i].x;
			chunkInfo[BATCH_SIZE*3+heightmapChunkCount*2+2] = m_queue[i].z;
			heightmapChunkCount++;
		}
	}
 
	chunkInfo[32*3] = m_seed;
	
	m_chunkInfoBuffer.unmap();
	
	VkSemaphore semaphore = m_application->device.createSemaphore();
	
	m_cmdBuffer.begin();
	
	m_cmdBuffer.bindPipeline(m_heightmapPipeline);
	m_cmdBuffer.bindDescriptorSet(m_heightmapDescSet);
	
	m_cmdBuffer.dispatch(batchSize, 1, 1);
	
	m_cmdBuffer.end();
	
	tm.pause();
	m_application->computeQueue->submitAndWait(&m_cmdBuffer);
	tm.unpause();
	
	//m_application->computeQueue->submit(&m_cmdBuffer, VK_NULL_HANDLE, &semaphore);
	
	m_cmdBuffer.begin();
	
	//m_cmdBuffer.bufferBarrier(&m_heightmapBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	
	m_cmdBuffer.bindPipeline(m_terrainPipeline);
	m_cmdBuffer.bindDescriptorSet(m_terrainDescSet);
	
	m_cmdBuffer.dispatch(batchSize, 1, 1);
	
	m_cmdBuffer.end();
	
	tm.pause();
	m_application->computeQueue->submit(&m_cmdBuffer, VK_NULL_HANDLE, &semaphore);
	tm.unpause();
	
	m_transferCmdBuffer.begin();
	 
	VkBufferCopy bufferCopy;
	bufferCopy.srcOffset = 0;
	bufferCopy.dstOffset = 0;
	bufferCopy.size = sizeof(uint8_t) * 16 * 16 * 16 * batchSize;
	
	m_transferCmdBuffer.copyBuffer(&m_stagingBuffer, &m_resultBuffer, bufferCopy);
	
	m_transferCmdBuffer.end();
	
	VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	
	tm.pause();
	m_application->transferQueue->submitAndWait(&m_transferCmdBuffer, 1, &semaphore, &pipelineStageFlags);
	tm.unpause();
	
	m_application->device.destroySempahore(semaphore);
	
	uint8_t* cubes = (uint8_t*)m_stagingBuffer.map();
	
	for(int i = 0; i < batchSize; i++) {
		m_chunks[m_chunkCount].init(this, m_queue[startIndex + i], &cubes[16 * 16 * 16 * i], m_chunkCount);
		m_chunkCount++;
	}
	
	m_stagingBuffer.unmap();
}

void ChunkManager::flushQueue() {
	Timer tm("TM");
	Timer cpuTM("CPU");
	
	
	for(int i = 0; i < m_queue.size(); i += BATCH_SIZE) {
		tm.start();
		cpuTM.start();
		processBatch(i, fmin(BATCH_SIZE, m_queue.size() - i), cpuTM);
		tm.stop();
		cpuTM.stop();
	}
	
	cpuTM.printLapTime();
	cpuTM.reset();
	
	tm.printLapTime();
	tm.reset();
	
	
	for(int i = 0; i < m_chunkCount; i++) {
		tm.start();
		m_chunks[i].update();
		tm.stop();
	}
	tm.printLapTime();
	tm.reset();
	m_queue.clear();
	
	if(m_faceNum == 0) {
		printf("No cube faces generated!\n");
	} else {
		m_facesBuffer.uploadData(m_application->transferQueue, m_facesBuff, sizeof(uint32_t) * m_faceNum, 0);
		m_positionBuffer.uploadData(m_application->transferQueue, m_positionBuff, sizeof(uint32_t) * m_faceNum * 3, 0);
	}
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
	
	m_resultBuffer.destroy();
	m_stagingBuffer.destroy();
	
	m_facesBuffer.destroy();
	m_facesStagingBuffer.destroy();
	
	m_positionBuffer.destroy();
	m_chunkInfoBuffer.destroy();
	m_heightmapBuffer.destroy();
	
	m_transferCmdBuffer.destroy();
	m_cmdBuffer.destroy();
	
	m_terrainDescSet->destroy();
	delete m_terrainDescSet;
	
	m_heightmapDescSet->destroy();
	delete m_heightmapDescSet;
	

	delete[] m_chunks;
	delete[] m_cubeNoises;
	
	m_terrainPipeline.destroy();
	m_terrainLayout.destroy();
	
	m_heightmapPipeline.destroy();
	m_heightmapLayout.destroy();
}
