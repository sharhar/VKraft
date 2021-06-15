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
	VKLBuffer* m_instBuffer;
	Vec3i m_pos;
	int m_cubes[16*16*16];
};

#endif /* Chunk_h */
