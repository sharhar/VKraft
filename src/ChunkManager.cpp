#include "Application.h"
#include "ChunkManager.h"
#include "FastNoise.h"
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

struct PC {
	float x, y, z;
	int seed;
};

ChunkManager::ChunkManager(Application* application) : m_cmdBuffer(application->computeQueue), m_transferCmdBuffer(application->transferQueue){
	m_application = application;
	
	m_chunks = new Chunk[16*16*16];
	m_chunkCount = 0;
	
	size_t shaderSize = 0;
	uint32_t* shaderCode = (uint32_t*)FileUtils::readBinaryFile("res/world-gen.spv", &shaderSize);
	
	m_layout.create(VKLPipelineLayoutCreateInfo()
							.device(&application->device)
							.addShaderModule(shaderCode, shaderSize, VK_SHADER_STAGE_COMPUTE_BIT, "main")
							.addDescriptorSet()
								.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT)
							.end()
							.addPushConstant(VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(struct PC)));
	
	m_computePipeline.create(VKLPipelineCreateInfo().layout(&m_layout));
	
	m_seed = 1337;
	
	m_heightNoise = new FastNoise(m_seed);
	m_caveNoise = new FastNoise(m_seed);
	m_caveNoise->SetFrequency(0.02);
	m_caveNoise->SetCellularDistanceFunction(FastNoise::Euclidean);
	m_caveNoise->SetCellularReturnType(FastNoise::Distance2Add);
	m_oreNoise = new FastNoise(m_seed);
	
	m_cubeNoises = new CubeNoise[16 * 16 * 16];
	
	m_resultBuffer.create(VKLBufferCreateInfo()
						  .size(sizeof(uint8_t) * 16 * 16 * 16 * BATCH_SIZE)
								.device(&application->device)
								.usage(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
								.memoryUsage(VMA_MEMORY_USAGE_GPU_ONLY));
	
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
	
	m_descSet = new VKLDescriptorSet(&m_layout, 0);
	
	m_descSet->writeBuffer(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &m_resultBuffer, 0, sizeof(uint8_t) * 16 * 16 * 16 * BATCH_SIZE);
}

uint32_t ChunkManager::getChunkCount() {
	return m_chunkCount;
}

void ChunkManager::queueChunk(MathUtils::Vec3i pos) {
	m_queue.push_back(pos);
}

void ChunkManager::processBatch(int startIndex, int batchSize) {
	struct PC pc;
	pc.x = 0;//worldPos.x;
	pc.y = 0;//worldPos.y;
	pc.z = 0;//worldPos.z;
	
	pc.seed = 3;
	
	VkSemaphore semaphore = m_application->device.createSemaphore();
	
	m_cmdBuffer.begin();
	
	m_cmdBuffer.bindPipeline(m_computePipeline);
	m_cmdBuffer.bindDescriptorSet(m_descSet);
	m_cmdBuffer.pushConstants(m_computePipeline, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(struct PC), &pc);
	
	m_cmdBuffer.dispatch(batchSize, 1, 1);
	
	m_cmdBuffer.end();
	
	m_application->computeQueue->submit(&m_cmdBuffer, VK_NULL_HANDLE, &semaphore);
	
	m_transferCmdBuffer.begin();
	 
	VkBufferCopy bufferCopy;
	bufferCopy.srcOffset = 0;
	bufferCopy.dstOffset = 0;
	bufferCopy.size = sizeof(uint8_t) * 16 * 16 * 16 * batchSize;
	
	m_transferCmdBuffer.copyBuffer(&m_stagingBuffer, &m_resultBuffer, bufferCopy);
	
	m_transferCmdBuffer.end();
	
	VkPipelineStageFlags pipelineStageFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	
	m_application->transferQueue->submitAndWait(&m_transferCmdBuffer, 1, &semaphore, &pipelineStageFlags);
	
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
	
	tm.start();
	for(int i = 0; i < m_queue.size(); i += BATCH_SIZE) {
		
		processBatch(i, fmin(BATCH_SIZE, m_queue.size() - i));
		
	}
	
	tm.stop();
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
	
	
	m_facesBuffer.uploadData(m_application->transferQueue, m_facesBuff, sizeof(uint32_t) * m_faceNum, 0);
	m_positionBuffer.uploadData(m_application->transferQueue, m_positionBuff, sizeof(uint32_t) * m_faceNum * 3, 0);
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
	//m_positionIndexBuffer.destroy();
	
	m_transferCmdBuffer.destroy();
	m_cmdBuffer.destroy();
	
	m_descSet->destroy();
	delete m_descSet;

	delete[] m_chunks;
	delete m_heightNoise;
	delete m_caveNoise;
	delete m_oreNoise;
	delete[] m_cubeNoises;
	
	m_computePipeline.destroy();
	m_layout.destroy();
}
