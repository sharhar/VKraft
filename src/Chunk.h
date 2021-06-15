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
	Chunk(VKLDevice* device, Vec3i pos);
	void render(VkCommandBuffer cmdBuffer);
	void destroy();
private:
	VKLDevice* m_device;
	VKLBuffer* m_instBuffer;
	Vec3i m_pos;
	int m_cubes[16*16*16];
	std::vector<int> m_renderCubes;
	
	void calcRenderCubes();
	int getCubeAt(int x, int y, int z);
};

#endif /* Chunk_h */
