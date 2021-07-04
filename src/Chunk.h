//
//  Chunk.h
//  VKraft
//
//  Created by Shahar Sandhaus on 6/12/21.
//

#ifndef Chunk_h
#define Chunk_h

#include <VKL/VKL.h>
#include <vector>
#include "Utils.h"

class Chunk {
public:
	static void init(VKLDevice* device);
	
	Chunk(Vec3i pos);
	void render(VkCommandBuffer cmdBuffer);
	void destroy();
	
	uint8_t atPos(Vec3i pos);
	
	Vec3 renderPos;
private:
	static VKLDevice* m_device;
	VKLBuffer* m_instBuffer;
	Vec3i m_pos;
	uint32_t m_cubes[16*16*16];
	std::vector<uint32_t> m_renderCubes;
	
	void calcRenderCubes();
	void calcChunkBorderData();
	void setChunkBorderBit(int borderID, int cubePos, uint32_t value);
	uint32_t getChunkBorderBit(int borderID, int cubePos);
	uint32_t getCubeAt(int x, int y, int z);
	
	uint64_t* m_adjChunkBorders[6];
	
	uint64_t m_chunkBorderData[4*6];
};

#endif /* Chunk_h */
