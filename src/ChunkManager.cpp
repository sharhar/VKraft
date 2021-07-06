//
//  ChunkManager.cpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/24/21.
//

#include "ChunkManager.h"

std::vector<Chunk> ChunkManager::m_chunks = std::vector<Chunk>();

const std::vector<Chunk>& ChunkManager::getChunks() {
	return m_chunks;
}

void ChunkManager::addChunk(Vec3i pos) {
	m_chunks.push_back(Chunk(pos));
	m_chunks.back().updateNearChunks();
}

int ChunkManager::getChunkAt(Vec3i pos) {
	for(int i = 0; i < m_chunks.size(); i++) {
		if(m_chunks[i].atPos(pos)) {
			return i;
		}
	}
	
	return -1;
}

Chunk* ChunkManager::getChunkFromIndex(int index) {
	if(index == -1) {
		return NULL;
	}
	
	return &(m_chunks.data()[index]);
}
