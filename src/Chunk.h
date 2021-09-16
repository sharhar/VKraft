#ifndef Chunk_h
#define Chunk_h

#include "Base.h"
#include <VKL/VKL.h>
#include <vector>
#include "Utils.h"

class Chunk {
public:
	Chunk();
	void init(ChunkManager* chunkManager, MathUtils::Vec3i pos, uint8_t* cubes, uint32_t index);
	void update();
	void destroy();

	uint8_t atPos(MathUtils::Vec3i pos);
	
	MathUtils::Vec3 renderPos;
private:
	ChunkManager* m_chunkManager;
	
	MathUtils::Vec3i m_pos;
	uint8_t m_cubes[16*16*16];
	
	uint32_t m_facesIndex;
	uint32_t m_chunkIndex;
	
	void calcRenderCubes();
	uint32_t isCubeTrasparent(int x, int y, int z);
};

#endif /* Chunk_h */
