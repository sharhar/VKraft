//
//  Chunk.h
//  VKraft
//
//  Created by Shahar Sandhaus on 6/12/21.
//

#ifndef Chunk_h
#define Chunk_h

#include <VKL/VKL.h>
#include "Utils.h"

class Chunk {
public:
	Chunk(VKLDevice* device, Vec3i pos);
	void bindInstanceBuffer(VkCommandBuffer cmdBuffer);
	void destroy();
private:
	VKLDevice* m_device;
	VKLBuffer* m_vertBuffer;
	Vec3i m_pos;
	uint16_t m_cubes[16*16*16];
	
	void genChunkMesh();
};

#endif /* Chunk_h */
