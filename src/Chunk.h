#ifndef Chunk_h
#define Chunk_h

#include "Base.h"
#include <VKL/VKL.h>
#include <vector>
#include "Utils.h"

class Chunk {
public:
	Chunk();
	void init(ChunkManager* chunkManager, MathUtils::Vec3i pos);
	void render(const VKLCommandBuffer* cmdBuffer);
	void updateNearChunks();
	void destroy();

	uint8_t renderable();
	uint8_t atPos(MathUtils::Vec3i pos);
	
	MathUtils::Vec3 renderPos;
private:
	ChunkManager* m_chunkManager;
	VKLBuffer m_instanceBuffer;
	
	MathUtils::Vec3i m_pos;
	uint8_t m_cubes[16*16*16];
	std::vector<uint32_t> m_renderCubes;
	
	void calcRenderCubes();
	void getChunkBorders();
	void update();
	void calcChunkBorderData();
	void setChunkBorderBit(int borderID, int cubePos, uint8_t value);
	uint32_t getChunkBorderBit(int borderID, int cubePos);
	uint32_t isCubeTrasparent(int x, int y, int z);
	
	uint64_t* m_adjChunkBorders[6];
	
	uint64_t m_chunkBorderData[4*6];
	
	Chunk* m_neighborChunks[6];
	
	uint8_t m_foundAllBorders;
};

#endif /* Chunk_h */
