#ifndef ChunkManager_h
#define ChunkManager_h

#include <stdio.h>
#include <vector>

#include "Base.h"

#include "Chunk.h"

typedef struct CubeNoise;

class ChunkManager {
public:
	ChunkManager(Application* application);
	
	uint32_t getChunkCount();
	void queueChunk(MathUtils::Vec3i pos);
	int getChunkAt(MathUtils::Vec3i pos);
	Chunk* getChunkFromIndex(int index);
	
	void flushQueue();
	
	void destroy();
private:
	Application* m_application;
	
	VKLPipelineLayout m_heightmapLayout;
	VKLPipeline m_heightmapPipeline;
	
	VKLPipelineLayout m_terrainLayout;
	VKLPipeline m_terrainPipeline;
	
	VKLBuffer m_resultBuffer;
	VKLBuffer m_chunkInfoBuffer;
	VKLBuffer m_stagingBuffer;
	
	VKLBuffer m_heightmapBuffer;
	
	VKLBuffer m_facesBuffer;
	VKLBuffer m_facesStagingBuffer;
	
	VKLBuffer m_positionBuffer;
	
	uint32_t m_faceNum;
	uint32_t m_facesBuff[16*16*16*3*512];
	
	int32_t m_positionBuff[16*16*16*3*512*3];
	
	std::vector<MathUtils::Vec3i> m_queue;
	
	VKLCommandBuffer m_cmdBuffer;
	VKLCommandBuffer m_transferCmdBuffer;
	VKLDescriptorSet* m_terrainDescSet;
	VKLDescriptorSet* m_heightmapDescSet;
	
	Chunk* m_chunks;
	uint32_t m_chunkCount;
	
	unsigned int m_seed;
	
	CubeNoise* m_cubeNoises;
	
	void processBatch(int startIndex, int batchSize, Timer& tm);
	
	friend class Chunk;
	friend class ChunkRenderer;
};

#endif /* ChunkManager_h */
