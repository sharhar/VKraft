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
	static const std::vector<Chunk>& getChunks();
	static void addChunk(Vec3i pos);
	static int getChunkAt(Vec3i pos);
	static Chunk* getChunkFromIndex(int index);
private:
	static std::vector<Chunk> m_chunks;
};

#endif /* ChunkManager_h */
