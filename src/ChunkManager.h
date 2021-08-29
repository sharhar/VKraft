#ifndef ChunkManager_h
#define ChunkManager_h

#include <stdio.h>
#include <vector>

#include "Base.h"

#include "Chunk.h"

class ChunkManager {
public:
	ChunkManager(Application* application);
	
	uint32_t getChunkCount();
	void addChunk(MathUtils::Vec3i pos);
	int getChunkAt(MathUtils::Vec3i pos);
	Chunk* getChunkFromIndex(int index);
	
	void destroy();
private:
	Application* m_application;
	
	Chunk* m_chunks;
	uint32_t m_chunkCount;
	
	friend class Chunk;
};

#endif /* ChunkManager_h */
