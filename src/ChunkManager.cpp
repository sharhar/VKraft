#include "Application.h"
#include "ChunkManager.h"

ChunkManager::ChunkManager(Application* application) {
	m_application = application;
	
	m_chunks = new Chunk[16*16*16];
	m_chunkCount = 0;
}

uint32_t ChunkManager::getChunkCount() {
	return m_chunkCount;
}

void ChunkManager::addChunk(MathUtils::Vec3i pos) {
	m_chunkCount++;
	m_chunks[m_chunkCount-1].init(this, pos);
	m_chunks[m_chunkCount-1].updateNearChunks();
}

int ChunkManager::getChunkAt(MathUtils::Vec3i pos) {
	for(int i = 0; i < m_chunkCount; i++) {
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
	
	return &(m_chunks[index]);
}

void ChunkManager::destroy() {
	for (int i = 0; i < m_chunkCount; i++) {
		m_chunks[i].destroy();
	}

	delete[] m_chunks;
}
