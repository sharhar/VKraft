//
//  Chunk.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/12/21.
//

#include "Chunk.h"
#include "ChunkManager.h"
#include "FastNoise.h"

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

//VKLDevice* Chunk::m_device = NULL;

static int getProp(int num, int mask, int exp) {
	return (num >> exp) & mask;
}

static int setProp(int num, int val, int mask, int exp) {
	return (val << exp) | (num & (~(mask << exp)));
}

//void Chunk::init(VKLDevice* device) {
//	m_device = device;
//}

typedef struct CubeNoise {
	float nz1;
	float nz2;

	float nz;

	float nzc;

	float cnzc;
	float inzc;
} CubeNoise;

inline float interpolateCubeValue(float val0, float val3, uint8_t rem) {
	float m = (val3 - val0)/3.0f;

	return val0 + m * rem;
}

Chunk::Chunk(Vec3i pos) {
	m_pos = pos;
	Vec3i worldPos = Vec3i(pos.x * 16, pos.y * 16, pos.z * 16);
	m_foundAllBorders = 0;
	
	renderPos = Vec3(pos.x * 16.0f, - pos.y * 16.0f, pos.z * 16.0f);
	
	unsigned int seed = 1337;
	
	FastNoise* heightNoise = new FastNoise(seed);
	FastNoise* caveNoise = new FastNoise(seed);
	caveNoise->SetFrequency(0.02);
	caveNoise->SetCellularDistanceFunction(FastNoise::Euclidean);
	caveNoise->SetCellularReturnType(FastNoise::Distance2Add);
	FastNoise* oreNoise = new FastNoise(seed);
	
	CubeNoise* cubeNoises = new CubeNoise[16 * 16 * 16];

	for (uint8_t x = 0; x < 16; x += 3) {
		float xf = worldPos.x + x;
		for (uint8_t z = 0; z < 16; z += 3) {
			float zf = worldPos.z + z;
			float nz1 = heightNoise->GetPerlin(xf / 1.2f, 0.8f, zf / 1.2f) * 64;
			float nz2 = heightNoise->GetPerlin(xf * 2, 2.7f, zf * 2) * 16;

			float nz = nz1 + nz2;

			for (uint8_t y = 0; y < 16; y += 3) {
				float yf = worldPos.y + y;

				float yh = yf - nz;

				if (yh <= 0) {
					float nzc = caveNoise->GetCellular(xf, yf, zf);

					cubeNoises[x * 256 + y * 16 + z].nzc = nzc;

					if (nzc < 0.45 && yh <= -5) {
						float cnzc = oreNoise->GetSimplex(xf / 0.5f, yf / 0.5f, zf / 0.5f);
						float inzc = oreNoise->GetSimplex(xf / 0.25f, yf / 0.25f, zf / 0.25f);

						cubeNoises[x * 256 + y * 16 + z].cnzc = cnzc;
						cubeNoises[x * 256 + y * 16 + z].inzc = inzc;
					}
				}

				cubeNoises[x * 256 + y * 16 + z].nz1 = nz1;
				cubeNoises[x * 256 + y * 16 + z].nz2 = nz2;
				cubeNoises[x * 256 + y * 16 + z].nz = nz;
			}
		}
	}
	
	for (uint8_t x = 0; x < 16; x += 1) {
		for (uint8_t z = 0; z < 16; z += 3) {
			for (uint8_t y = 0; y < 16; y += 3) {
				uint8_t rem = x % 3;
				if (rem != 0) {
					uint8_t off = x - rem;

					cubeNoises[x * 256 + y * 16 + z].nz1 = interpolateCubeValue(
						cubeNoises[(off) * 256 + y * 16 + z].nz1, cubeNoises[(off + 3) * 256 + y * 16 + z].nz1, rem);

					cubeNoises[x * 256 + y * 16 + z].nz2 = interpolateCubeValue(
						cubeNoises[(off) * 256 + y * 16 + z].nz2, cubeNoises[(off + 3) * 256 + y * 16 + z].nz2, rem);

					cubeNoises[x * 256 + y * 16 + z].nz = interpolateCubeValue(
						cubeNoises[(off) * 256 + y * 16 + z].nz, cubeNoises[(off+ 3) * 256 + y * 16 + z].nz, rem);

					cubeNoises[x * 256 + y * 16 + z].nzc = interpolateCubeValue(
						cubeNoises[(off) * 256 + y * 16 + z].nzc, cubeNoises[(off + 3) * 256 + y * 16 + z].nzc, rem);

					cubeNoises[x * 256 + y * 16 + z].cnzc = interpolateCubeValue(
						cubeNoises[(off) * 256 + y * 16 + z].cnzc, cubeNoises[(off + 3) * 256 + y * 16 + z].cnzc, rem);

					cubeNoises[x * 256 + y * 16 + z].inzc = interpolateCubeValue(
						cubeNoises[(off) * 256 + y * 16 + z].inzc, cubeNoises[(off + 3) * 256 + y * 16 + z].inzc, rem);
				}
			}
		}
	}

	for (uint8_t x = 0; x < 16; x += 1) {
		for (uint8_t z = 0; z < 16; z += 1) {
			for (uint8_t y = 0; y < 16; y += 3) {
				uint8_t rem = z % 3;
				if (rem != 0) {
					uint8_t off = z - rem;

					cubeNoises[x * 256 + y * 16 + z].nz1 = interpolateCubeValue(
						cubeNoises[x * 256 + y * 16 + (off)].nz1, cubeNoises[x * 256 + y * 16 + (off + 3)].nz1, rem);

					cubeNoises[x * 256 + y * 16 + z].nz2 = interpolateCubeValue(
						cubeNoises[x * 256 + y * 16 + (off)].nz2, cubeNoises[x * 256 + y * 16 + (off + 3)].nz2, rem);

					cubeNoises[x * 256 + y * 16 + z].nz = interpolateCubeValue(
						cubeNoises[x * 256 + y * 16 + (off)].nz, cubeNoises[x * 256 + y * 16 + (off + 3)].nz, rem);

					cubeNoises[x * 256 + y * 16 + z].nzc = interpolateCubeValue(
						cubeNoises[x * 256 + y * 16 + (off)].nzc, cubeNoises[x * 256 + y * 16 + (off + 3)].nzc, rem);

					cubeNoises[x * 256 + y * 16 + z].cnzc = interpolateCubeValue(
						cubeNoises[x * 256 + y * 16 + (off)].cnzc, cubeNoises[x * 256 + y * 16 + (off + 3)].cnzc, rem);

					cubeNoises[x * 256 + y * 16 + z].inzc = interpolateCubeValue(
						cubeNoises[x * 256 + y * 16 + (off)].inzc, cubeNoises[x * 256 + y * 16 + (off + 3)].inzc, rem);
				}
			}
		}
	}

	for (uint8_t x = 0; x < 16; x += 1) {
		for (uint8_t z = 0; z < 16; z += 1) {
			for (uint8_t y = 0; y < 16; y += 1) {
				uint8_t rem = y % 3;
				if (rem != 0) {
					uint8_t off = y - rem;

					cubeNoises[x * 256 + y * 16 + z].nz1 = interpolateCubeValue(
						cubeNoises[x * 256 + (off)* 16 + z].nz1, cubeNoises[x * 256 + (off + 3) * 16 + z].nz1, rem);

					cubeNoises[x * 256 + y * 16 + z].nz2 = interpolateCubeValue(
						cubeNoises[x * 256 + (off)* 16 + z].nz2, cubeNoises[x * 256 + (off + 3) * 16 + z].nz2, rem);

					cubeNoises[x * 256 + y * 16 + z].nz = interpolateCubeValue(
						cubeNoises[x * 256 + (off)* 16 + z].nz, cubeNoises[x * 256 + (off + 3) * 16 + z].nz, rem);

					cubeNoises[x * 256 + y * 16 + z].nzc = interpolateCubeValue(
						cubeNoises[x * 256 + (off)* 16 + z].nzc, cubeNoises[x * 256 + (off + 3) * 16 + z].nzc, rem);

					cubeNoises[x * 256 + y * 16 + z].cnzc = interpolateCubeValue(
						cubeNoises[x * 256 + (off)* 16 + z].cnzc, cubeNoises[x * 256 + (off + 3) * 16 + z].cnzc, rem);

					cubeNoises[x * 256 + y * 16 + z].inzc = interpolateCubeValue(
						cubeNoises[x * 256 + (off)* 16 + z].inzc, cubeNoises[x * 256 + (off + 3) * 16 + z].inzc, rem);
				}
			}
		}
	}
	
	for (uint8_t x = 0; x < 16; x++) {
		float xf = worldPos.x + x;
		for (uint8_t z = 0; z < 16; z++) {
			float zf = worldPos.z + z;

			for (uint8_t y = 0; y < 16; y++) {
				float yf = worldPos.y + y;

				float yh = yf - cubeNoises[x * 256 + y * 16 + z].nz;

				int type = 0;

				if (yh <= 0) {
					if (cubeNoises[x * 256 + y * 16 + z].nzc < 0.45) {
						if (yh <= 0 && yh > -1) {
							type = 1;
						}
						else if (yh <= -1 && yh > -5) {
							type = 2;
						}
						else {
							if (cubeNoises[x * 256 + y * 16 + z].cnzc > 0.8) {
								type = 4;
							}
							else if (cubeNoises[x * 256 + y * 16 + z].inzc > 0.8) {
								type = 5;
							}
							else {
								type = 3;
							}

						}
					} else {
						type = 0;
					}
				}
				
				m_cubes[x * 256 + y * 16 + z] = setProp(0, x * 256 + y * 16 + z, PROP_POS_MASK, PROP_POS_EXP);
				m_cubes[x * 256 + y * 16 + z] = setProp(m_cubes[x * 256 + y * 16 + z], type, PORP_ID_MASK, PORP_ID_EXP);
			}
		}
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
			//vklCreateStagedBuffer(m_device->deviceGraphicsContexts[0], &m_instBuffer,  m_renderCubes.data(), sizeof(int) * m_renderCubes.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
		}
	}
}

void Chunk::getChunkBorders() {
	if(m_foundAllBorders == 0) {
		int found_count = 0;
				
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
		//m_device->pvkCmdBindVertexBuffers(cmdBuffer, 1, 1, &m_instBuffer->buffer, &offsets);
		//m_device->pvkCmdDraw(cmdBuffer, 6, m_renderCubes.size(), 0, 0);
	}
}

void Chunk::destroy() {
	//vklDestroyBuffer(m_device, m_instBuffer);
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
						m_renderCubes.push_back(setProp(cube, 2, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (isCubeTrasparent(x+1, y, z)) {
						m_renderCubes.push_back(setProp(cube, 3, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (isCubeTrasparent(x, y, z-1)) {
						m_renderCubes.push_back(setProp(cube, 1, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (isCubeTrasparent(x, y, z+1)) {
						m_renderCubes.push_back(setProp(cube, 0, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (isCubeTrasparent(x, y-1, z)) {
						m_renderCubes.push_back(setProp(cube, 4, PROP_FACE_MASK, PROP_FACE_EXP));
					}
					
					if (isCubeTrasparent(x, y+1, z)) {
						m_renderCubes.push_back(setProp(cube, 5, PROP_FACE_MASK, PROP_FACE_EXP));
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
