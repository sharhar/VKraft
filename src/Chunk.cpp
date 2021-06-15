//
//  Chunk.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/12/21.
//

#include "Chunk.h"

#define PROP_FACE_MASK 0b111111
#define PROP_FACE_EXP 0

#define PROP_POS_MASK 0xfff
#define PROP_POS_EXP 6

#define PORP_ID_MASK 0xfff
#define PORP_ID_EXP 18

static int getProp(int num, int mask, int exp) {
	return (num >> exp) & mask;
}

static int setProp(int num, int val, int mask, int exp) {
	return (val << exp) | (num & (~(mask << exp)));
}

Chunk::Chunk(VKLDevice* device, Vec3i pos) {
	m_device = device;
	m_pos = pos;
	
	for(int i = 0; i < 16 * 16 * 16; i++) {
		int x = i % 16;
		int y = (i >> 4) % 16;
		
		m_cubes[i] = setProp(0, i, PROP_POS_MASK, PROP_POS_EXP);
		m_cubes[i] = setProp(m_cubes[i], 1, PORP_ID_MASK, PORP_ID_EXP);
		m_cubes[i] = setProp(m_cubes[i], i % 64, PROP_FACE_MASK, PROP_FACE_EXP);
	}
	
	vklCreateBuffer(m_device, &m_instBuffer, VK_FALSE, sizeof(int) * 16 * 16 * 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	vklWriteToMemory(m_device, m_instBuffer->memory, m_cubes, sizeof(int) * 16 * 16 * 16, 0);
}

void Chunk::bindInstanceBuffer(VkCommandBuffer cmdBuffer) {
	VkDeviceSize offsets = 0;
	m_device->pvkCmdBindVertexBuffers(cmdBuffer, 1, 1, &m_instBuffer->buffer, &offsets);
}

void Chunk::destroy() {
	vklDestroyBuffer(m_device, m_instBuffer);
}
