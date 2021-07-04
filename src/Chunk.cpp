//
//  Chunk.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/12/21.
//

#include "Chunk.h"
#include "ChunkManager.h"

#define PROP_FACE_MASK 7
#define PROP_FACE_EXP 0

#define PROP_POS_MASK 0xfff
#define PROP_POS_EXP 6

#define PORP_ID_MASK 0xfff
#define PORP_ID_EXP 18

#define TOP_CHUNK_BORDER 0
#define BOTTOM_CHUNK_BORDER 1

#define RIGHT_CHUNK_BORDER 2
#define LEFT_CHUNK_BORDER 3

#define BACK_CHUNK_BORDER 4
#define FRONT_CHUNK_BORDER 5

VKLDevice* Chunk::m_device = NULL;

static int getProp(int num, int mask, int exp) {
	return (num >> exp) & mask;
}

static int setProp(int num, int val, int mask, int exp) {
	return (val << exp) | (num & (~(mask << exp)));
}

void Chunk::init(VKLDevice* device) {
	m_device = device;
}

Chunk::Chunk(Vec3i pos) {
	m_pos = pos;
	
	renderPos = Vec3(pos.x * 16.0f, - pos.y * 16.0f, pos.z * 16.0f);
	
	for(int i = 0; i < 16 * 16 * 16; i++) {
		int z = 16 * pos.z + (i % 16);
		int y = 16 * pos.y + ((i >> 4) % 16);
		int x = 16 * pos.x + (i >> 8);
		
		int func = x*x + z*z;
		
		int height = y + func;
		
		int id = 0;
		
		if(height == -1) {
			id = 4;
		} else if(height < -1 && height >= -3) {
			id = 4;
		} else if (height < -3) {
			id = 4;
		}
		
		m_cubes[i] = setProp(0, i, PROP_POS_MASK, PROP_POS_EXP);
		m_cubes[i] = setProp(m_cubes[i], id, PORP_ID_MASK, PORP_ID_EXP);
	}
	
	int botIndx = ChunkManager::getChunkAt(Vec3i(pos.x, pos.y-1, pos.z));
	
	if(botIndx != -1) {
		m_adjChunkBorders[BOTTOM_CHUNK_BORDER] =  &ChunkManager::getChunkFromIndex(botIndx).m_chunkBorderData[4*BOTTOM_CHUNK_BORDER];
	}
	
	calcRenderCubes();
	calcChunkBorderData();
	
	if(m_renderCubes.size()) {
		vklCreateStagedBuffer(m_device->deviceGraphicsContexts[0], &m_instBuffer,  m_renderCubes.data(), sizeof(int) * m_renderCubes.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	}
}

void Chunk::render(VkCommandBuffer cmdBuffer) {
	if(m_renderCubes.size()) {
		VkDeviceSize offsets = 0;
		m_device->pvkCmdBindVertexBuffers(cmdBuffer, 1, 1, &m_instBuffer->buffer, &offsets);
		m_device->pvkCmdDraw(cmdBuffer, 6, m_renderCubes.size(), 0, 0);
	}
}

void Chunk::destroy() {
	vklDestroyBuffer(m_device, m_instBuffer);
}

uint32_t Chunk::getCubeAt(int x, int y, int z) {
	if(y == -1) {
		return getChunkBorderBit(BOTTOM_CHUNK_BORDER, 16 * x + z);
	}
	
	if(x < 0 || x > 15 || y < -1 || y > 15 || z < 0 || z > 15) {
		return 0; // TODO: figure out true boundary conditions based on adjacent chunks
	}
	
	return m_cubes[x * 256 + y * 16 + z];
}

void Chunk::calcRenderCubes() {
	m_renderCubes.clear();
	
	for(int x = 0; x < 16; x++) {
		for(int y = 0; y < 16; y++) {
			for(int z = 0; z < 16; z++) {
				uint32_t cube = getCubeAt(x, y, z);
				
				if(getProp(cube, PORP_ID_MASK, PORP_ID_EXP)) {
					if (getProp(getCubeAt(x-1, y, z), PORP_ID_MASK, PORP_ID_EXP) == 0) {
						int ID = setProp(cube, 3, PORP_ID_MASK, PORP_ID_EXP);
						m_renderCubes.push_back(setProp(ID, 2, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (getProp(getCubeAt(x+1, y, z), PORP_ID_MASK, PORP_ID_EXP) == 0) {
						int ID = setProp(cube, 2, PORP_ID_MASK, PORP_ID_EXP);
						m_renderCubes.push_back(setProp(ID, 3, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (getProp(getCubeAt(x, y, z-1), PORP_ID_MASK, PORP_ID_EXP) == 0) {
						int ID = setProp(cube, 8, PORP_ID_MASK, PORP_ID_EXP);
						m_renderCubes.push_back(setProp(ID, 1, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (getProp(getCubeAt(x, y, z+1), PORP_ID_MASK, PORP_ID_EXP) == 0) {
						int ID = setProp(cube, 1, PORP_ID_MASK, PORP_ID_EXP);
						m_renderCubes.push_back(setProp(ID, 0, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (getProp(getCubeAt(x, y-1, z), PORP_ID_MASK, PORP_ID_EXP) == 0) {
						int ID = setProp(cube, 6, PORP_ID_MASK, PORP_ID_EXP);
						m_renderCubes.push_back(setProp(ID, 4, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (getProp(getCubeAt(x, y+1, z), PORP_ID_MASK, PORP_ID_EXP) == 0) {
						int ID = setProp(cube, 5, PORP_ID_MASK, PORP_ID_EXP);
						m_renderCubes.push_back(setProp(ID, 5, PROP_FACE_MASK, PROP_FACE_EXP));
					}
				}
			}
		}
	}
}

void Chunk::setChunkBorderBit(int borderID, int cubePos, uint32_t value) {
	int arrayOffset = cubePos / 64;
	int bitOffset = cubePos % 64;
	
	uint64_t setVal = ((uint64_t)(value ? 1 : 0)) << bitOffset;
	uint64_t bitMask = ~(((uint64_t)1) << bitOffset);
	
	printf("V %llu\n", value);
	
	m_chunkBorderData[4*borderID + arrayOffset] = (m_chunkBorderData[4*borderID + arrayOffset] & bitMask) | setVal;
}

uint32_t Chunk::getChunkBorderBit(int borderID, int cubePos) {
	int arrayOffset = cubePos / 64;
	int bitOffset = cubePos % 64;
	
	return (m_chunkBorderData[4*borderID + arrayOffset] >> bitOffset) & 1;
}

void Chunk::calcChunkBorderData() {
	for(int x = 0; x < 16; x++) {
		for(int z = 0; z < 16; z++) {
			printf("c %d\n", getProp(getCubeAt(x, 15, z), PORP_ID_MASK, PORP_ID_EXP));
			
			setChunkBorderBit(BOTTOM_CHUNK_BORDER, x*16 + z,
							  getProp(getCubeAt(x, 15, z), PORP_ID_MASK, PORP_ID_EXP));
		}
	}
	
	printf("CHUNK\n");
	printf("%llu\n", m_chunkBorderData[BOTTOM_CHUNK_BORDER*4]);
}

uint8_t Chunk::atPos(Vec3i pos) {
	return m_pos.x == pos.x && m_pos.y == pos.y && m_pos.z == pos.z;
}
