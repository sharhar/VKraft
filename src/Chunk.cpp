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

Vec3i pos_offs[6] = {
	Vec3i(0, 1, 0),
	Vec3i(0, -1, 0),
	
	Vec3i(1, 0, 0),
	Vec3i(-1, 0, 0),
	
	Vec3i(0, 0, -1),
	Vec3i(0, 0, 1),
};

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
	m_foundAllBorders = 0;
	
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
	
	memset(m_chunkBorderData, 0, sizeof(uint64_t) * 4 * 6);
	m_foundAllBorders = 0;
	
	calcChunkBorderData();
	
	for(int i = 0; i < 6; i++) {
		m_neighborChunks[i] = ChunkManager::getChunkFromIndex(ChunkManager::getChunkAt(m_pos.add(pos_offs[i])));
	}
	
	update();
}

void Chunk::updateNearChunks() {
	for(int i = 0; i < 6; i++) {
		if(m_neighborChunks[i] != NULL) {
			m_neighborChunks[i]->update();
		}
	}
}

void Chunk::update() {
	getChunkBorders();
	
	if(m_foundAllBorders == 1) {
		calcRenderCubes();
		
		if(m_renderCubes.size()) {
			vklCreateStagedBuffer(m_device->deviceGraphicsContexts[0], &m_instBuffer,  m_renderCubes.data(), sizeof(int) * m_renderCubes.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		}
	}
}

void Chunk::getChunkBorders() {
	if(m_foundAllBorders == 0) {
		int found_count = 0;
		
		//printf("Update: %s\n", m_pos.to_string().c_str());
		
		for(int i = 0; i < 6; i++) {
			if(m_neighborChunks[i] == NULL) {
				m_neighborChunks[i] = ChunkManager::getChunkFromIndex(ChunkManager::getChunkAt(m_pos.add(pos_offs[i])));
				
				if(m_neighborChunks[i] != NULL) {
					//printf("ID2: %d\n", i);
				}
			}
			
			if(m_neighborChunks[i] != NULL) {
				found_count++;
			}
		}
		
		if(found_count == 6) {
			m_foundAllBorders = 1;
		}
	}
	
	if(m_foundAllBorders == 1) {
		for(int i = 0; i < 6;i++) {
			m_adjChunkBorders[i] = &m_neighborChunks[i]->m_chunkBorderData[4*i];
		}
	}
}

void Chunk::render(VkCommandBuffer cmdBuffer) {
	if(m_foundAllBorders && m_renderCubes.size()) {
		VkDeviceSize offsets = 0;
		m_device->pvkCmdBindVertexBuffers(cmdBuffer, 1, 1, &m_instBuffer->buffer, &offsets);
		m_device->pvkCmdDraw(cmdBuffer, 6, m_renderCubes.size(), 0, 0);
	}
}

void Chunk::destroy() {
	vklDestroyBuffer(m_device, m_instBuffer);
}

uint32_t Chunk::isCubeTrasparent(int x, int y, int z) {
	if(x == -1) {
		return getChunkBorderBit(LEFT_CHUNK_BORDER, 16 * y + z);
	}
	
	if(x == 16) {
		return getChunkBorderBit(RIGHT_CHUNK_BORDER, 16 * y + z);
	}
	
	if(y == -1) {
		return getChunkBorderBit(BOTTOM_CHUNK_BORDER, 16 * x + z);
	}
	
	if(y == 16) {
		return getChunkBorderBit(TOP_CHUNK_BORDER, 16 * x + z);
	}
	
	if(z == -1) {
		return getChunkBorderBit(BACK_CHUNK_BORDER, 16 * x + y);
	}
	
	if(z == 16) {
		return getChunkBorderBit(FRONT_CHUNK_BORDER, 16 * x + y);
	}
	
	return getProp(m_cubes[x * 256 + y * 16 + z], PORP_ID_MASK, PORP_ID_EXP) == 0;
}

void Chunk::calcRenderCubes() {
	m_renderCubes.clear();
	
	for(int x = 0; x < 16; x++) {
		for(int y = 0; y < 16; y++) {
			for(int z = 0; z < 16; z++) {
				uint32_t cube = m_cubes[x * 256 + y * 16 + z];
				
				if(getProp(cube, PORP_ID_MASK, PORP_ID_EXP)) {
					if (isCubeTrasparent(x-1, y, z)) {
						int ID = setProp(cube, 3, PORP_ID_MASK, PORP_ID_EXP);
						m_renderCubes.push_back(setProp(ID, 2, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (isCubeTrasparent(x+1, y, z)) {
						int ID = setProp(cube, 2, PORP_ID_MASK, PORP_ID_EXP);
						m_renderCubes.push_back(setProp(ID, 3, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (isCubeTrasparent(x, y, z-1)) {
						int ID = setProp(cube, 8, PORP_ID_MASK, PORP_ID_EXP);
						m_renderCubes.push_back(setProp(ID, 1, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (isCubeTrasparent(x, y, z+1)) {
						int ID = setProp(cube, 1, PORP_ID_MASK, PORP_ID_EXP);
						m_renderCubes.push_back(setProp(ID, 0, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (isCubeTrasparent(x, y-1, z)) {
						int ID = setProp(cube, 6, PORP_ID_MASK, PORP_ID_EXP);
						m_renderCubes.push_back(setProp(ID, 4, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (isCubeTrasparent(x, y+1, z)) {
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
	
	uint64_t setVal = ((uint64_t)(value ? 0 : 1)) << bitOffset;
	uint64_t bitMask = ~(((uint64_t)1) << bitOffset);
	
	m_chunkBorderData[4*borderID + arrayOffset] = (m_chunkBorderData[4*borderID + arrayOffset] & bitMask) | setVal;
}

uint32_t Chunk::getChunkBorderBit(int borderID, int cubePos) {
	int arrayOffset = cubePos / 64;
	int bitOffset = cubePos % 64;
	
	return (m_adjChunkBorders[borderID][arrayOffset] >> bitOffset) & 1;}

void Chunk::calcChunkBorderData() {
	for(int x = 0; x < 16; x++) {
		for(int z = 0; z < 16; z++) {
			setChunkBorderBit(BOTTOM_CHUNK_BORDER, x * 16 + z, getProp(m_cubes[x * 256 + 15 * 16 + z], PORP_ID_MASK, PORP_ID_EXP));
			setChunkBorderBit(TOP_CHUNK_BORDER   , x * 16 + z, getProp(m_cubes[x * 256 +  0 * 16 + z], PORP_ID_MASK, PORP_ID_EXP));
		}
	}
	
	for(int y = 0; y < 16; y++) {
		for(int z = 0; z < 16; z++) {
			setChunkBorderBit(LEFT_CHUNK_BORDER , y * 16 + z, getProp(m_cubes[15 * 256 + y * 16 + z], PORP_ID_MASK, PORP_ID_EXP));
			setChunkBorderBit(RIGHT_CHUNK_BORDER, y * 16 + z, getProp(m_cubes[ 0 * 256 + y * 16 + z], PORP_ID_MASK, PORP_ID_EXP));
		}
	}
	
	for(int x = 0; x < 16; x++) {
		for(int y = 0; y < 16; y++) {
			setChunkBorderBit(BACK_CHUNK_BORDER , x * 16 + y, getProp(m_cubes[x * 256 + y * 16 + 15], PORP_ID_MASK, PORP_ID_EXP));
			setChunkBorderBit(FRONT_CHUNK_BORDER, x * 16 + y, getProp(m_cubes[x * 256 + y * 16 +  0], PORP_ID_MASK, PORP_ID_EXP));
		}
	}
}

uint8_t Chunk::atPos(Vec3i pos) {
	return m_pos.x == pos.x && m_pos.y == pos.y && m_pos.z == pos.z;
}
