//
//  Chunk.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/12/21.
//

#include "Chunk.h"

static uint8_t _getVid(uint16_t num) {
	return (uint8_t)(num & 0b0000000000111111);
}

static uint16_t _getType(uint16_t num) {
	return (uint16_t)((num & 0b1111111111000000) >> 6);
}

static uint16_t _setVid(uint16_t num, uint8_t vid) {
	return (num & 0b1111111111000000) +
		(((uint16_t)vid) & 0b0000000000111111);
}

static uint16_t _setType(uint16_t num, uint16_t type) {
	return (num & 0b0000000000111111) +
		((type & 0b0000001111111111) << 6);
}

Chunk::Chunk(VKLDevice* device, Vec3i pos) {
	m_device = device;
	m_pos = pos;
	
	uint16_t filled_cube = _setType(0, 1);
	
	memset(m_cubes, 0, sizeof(uint16_t) * 16 * 16 * 16);
	
	for(int i = 0; i < 16 * 16 * 16; i++) {
		m_cubes[i] = filled_cube;
	}
}

void Chunk::bindInstanceBuffer(VkCommandBuffer cmdBuffer) {
	
}

void Chunk::destroy() {
	
}
