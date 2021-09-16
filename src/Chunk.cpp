#include "Chunk.h"
#include "ChunkManager.h"
#include "Application.h"

#include "Utils.h"

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

using namespace MathUtils;

Vec3i pos_offs[6] = {
	Vec3i(0, 1, 0),
	Vec3i(0, -1, 0),
	
	Vec3i(1, 0, 0),
	Vec3i(-1, 0, 0),
	
	Vec3i(0, 0, -1),
	Vec3i(0, 0, 1),
};

static int getProp(int num, int mask, int exp) {
	return (num >> exp) & mask;
}

static int setProp(int num, int val, int mask, int exp) {
	return (val << exp) | (num & (~(mask << exp)));
}

Chunk::Chunk() {
	m_pos = Vec3i(0, 0, 0);
	renderPos = Vec3(0, 0, 0);
}

void Chunk::init(ChunkManager* chunkManager, Vec3i pos, uint8_t* cubes, uint32_t index) {
	m_chunkManager = chunkManager;
	
	m_chunkIndex = index;
	
	m_pos = pos;
	Vec3i worldPos = Vec3i(pos.x * 16, pos.y * 16, pos.z * 16);
	
	renderPos = Vec3(pos.x * 16.0f, - pos.y * 16.0f, pos.z * 16.0f);
	
	memcpy(m_cubes, cubes, sizeof(uint8_t) * 16 * 16 * 16);
	
	//chunkManager->genCubes(m_cubes, worldPos);
	
	//update();
}

void Chunk::update() {
	Timer tt("Render calc");
	
	tt.start();
	
	m_facesIndex = m_chunkManager->m_faceNum;
	
	calcRenderCubes();
	
	for (int i = 0; i < m_chunkManager->m_faceNum - m_facesIndex; i++) {
		m_chunkManager->m_positionBuff[3*(m_facesIndex + i)+0] = m_pos.x;
		m_chunkManager->m_positionBuff[3*(m_facesIndex + i)+1] = m_pos.y;
		m_chunkManager->m_positionBuff[3*(m_facesIndex + i)+2] = m_pos.z;
	}
	
	tt.stop();
	//tt.printLapTime();
}

void Chunk::destroy() {
	
}


inline uint32_t Chunk::isCubeTrasparent(int x, int y, int z) {
	if(x == -1) {
		return 1;
	}
	
	if(x == 16) {
		return 1;
	}
	
	if(y == -1) {
		return 1;
	}
	
	if(y == 16) {
		return 1;
	}
	
	if(z == -1) {
		return 1;
	}
	
	if(z == 16) {
		return 1;
	}
	
	return m_cubes[x * 256 + y * 16 + z] == 0;
}

inline uint32_t calcCubeFace(uint8_t type, uint8_t face, uint32_t pos) {
	uint32_t result = 0;
	result = setProp(result, face, PROP_FACE_MASK, PROP_FACE_EXP);
	result = setProp(result, pos, PROP_POS_MASK, PROP_POS_EXP);
	result = setProp(result, type, PORP_ID_MASK, PORP_ID_EXP);
	return result;
}

void Chunk::calcRenderCubes() {
	for(int x = 0; x < 16; x++) {
		for(int y = 0; y < 16; y++) {
			for(int z = 0; z < 16; z++) {
				uint32_t index = x * 256 + y * 16 + z;
				uint8_t cube = m_cubes[index];
				
				if (cube) {
					if (isCubeTrasparent(x-1, y, z)) {
						m_chunkManager->m_facesBuff[m_chunkManager->m_faceNum] = calcCubeFace(cube, 2, index);
						m_chunkManager->m_faceNum++;
					}
					
					if (isCubeTrasparent(x+1, y, z)) {
						m_chunkManager->m_facesBuff[m_chunkManager->m_faceNum] = calcCubeFace(cube, 3, index);
						m_chunkManager->m_faceNum++;
					}
					
					if (isCubeTrasparent(x, y, z-1)) {
						m_chunkManager->m_facesBuff[m_chunkManager->m_faceNum] = calcCubeFace(cube, 1, index);
						m_chunkManager->m_faceNum++;
					}
					
					if (isCubeTrasparent(x, y, z+1)) {
						m_chunkManager->m_facesBuff[m_chunkManager->m_faceNum] = calcCubeFace(cube, 0, index);
						m_chunkManager->m_faceNum++;
					}
					
					if (isCubeTrasparent(x, y-1, z)) {
						m_chunkManager->m_facesBuff[m_chunkManager->m_faceNum] = calcCubeFace(cube, 4, index);
						m_chunkManager->m_faceNum++;
					}
					
					if (isCubeTrasparent(x, y+1, z)) {
						m_chunkManager->m_facesBuff[m_chunkManager->m_faceNum] = calcCubeFace(cube, 5, index);
						m_chunkManager->m_faceNum++;
					}
				}
			}
		}
	}
}

uint8_t Chunk::atPos(Vec3i pos) {
	return m_pos.x == pos.x && m_pos.y == pos.y && m_pos.z == pos.z;
}
