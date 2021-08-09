//
//  ChunkManager.h
//  VKraft
//
//  Created by Shahar Sandhaus on 6/24/21.
//

#ifndef ChunkManager_h
#define ChunkManager_h

#include <stdio.h>
#include <vector>

#include "Chunk.h"

class ChunkManager {
public:
	static uint32_t getChunkCount();
	static void addChunk(Vec3i pos);
	static int getChunkAt(Vec3i pos);
	static Chunk* getChunkFromIndex(int index);
	static void init();
	static void destroy();
private:
	//static std::vector<Chunk> m_chunks;
	static Chunk* m_chunks;
	static uint32_t m_chunkCount;
};

#endif /* ChunkManager_h */
