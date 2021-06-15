//
//  Chunk.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/12/21.
//

#include "Chunk.h"

#define PROP_FACE_MASK 63
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
		m_cubes[i] = setProp(m_cubes[i], x < y, PORP_ID_MASK, PORP_ID_EXP);
	}
	
	calcRenderCubes();
	
	vklCreateStagedBuffer(m_device->deviceGraphicsContexts[0], &m_instBuffer,  m_renderCubes.data(), sizeof(int) * m_renderCubes.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	
	//vklCreateBuffer(m_device, &m_instBuffer, VK_FALSE, sizeof(int) * 16 * 16 * 16, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	//vklWriteToMemory(m_device, m_instBuffer->memory, m_renderCubes.data(), sizeof(int) * m_renderCubes.size(), 0);
}

void Chunk::render(VkCommandBuffer cmdBuffer) {
	VkDeviceSize offsets = 0;
	m_device->pvkCmdBindVertexBuffers(cmdBuffer, 1, 1, &m_instBuffer->buffer, &offsets);
	m_device->pvkCmdDraw(cmdBuffer, 36, m_renderCubes.size(), 0, 0);
}

void Chunk::destroy() {
	vklDestroyBuffer(m_device, m_instBuffer);
}

int Chunk::getCubeAt(int x, int y, int z) {
	if(x < 0 || x > 15 || y < 0 || y > 15 || z < 0 || z > 15) {
		return 0; // TODO: figure out true boundary conditions based on adjacent chunks
	}
	
	return m_cubes[z * 256 + y * 16 + x];
}

void Chunk::calcRenderCubes() {
	m_renderCubes.clear();
	
	for(int x = 0; x < 16; x++) {
		for(int y = 0; y < 16; y++) {
			for(int z = 0; z < 16; z++) {
				int cube = m_cubes[z * 256 + y * 16 + x];
				
				if(getProp(cube, PORP_ID_MASK, PORP_ID_EXP)) {
					int vid = 0;
					
					if (!getProp(getCubeAt(x-1, y, z), PORP_ID_MASK, PORP_ID_EXP)) {
						vid = vid | 1;
					}
					
					if (!getProp(getCubeAt(x+1, y, z), PORP_ID_MASK, PORP_ID_EXP)) {
						vid = vid | 2;
					}
					
					if (!getProp(getCubeAt(x, y, z-1), PORP_ID_MASK, PORP_ID_EXP)) {
						vid = vid | 4;
					}
					
					if (!getProp(getCubeAt(x, y, z+1), PORP_ID_MASK, PORP_ID_EXP)) {
						vid = vid | 8;
					}
					
					if (!getProp(getCubeAt(x, y-1, z), PORP_ID_MASK, PORP_ID_EXP)) {
						vid = vid | 16;
					}
					
					if (!getProp(getCubeAt(x, y+1, z), PORP_ID_MASK, PORP_ID_EXP)) {
						vid = vid | 32;
					}
					
					if(vid) {
						m_renderCubes.push_back(setProp(m_cubes[z * 256 + y * 16 + x], vid, PROP_FACE_MASK, PROP_FACE_EXP));
					}
				}
			}
		}
	}
}
