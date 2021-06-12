#include "VLKUtils.h"
#include "Cube.h"
#include "Camera.h"
#include <iostream>
#include <assert.h>

#define CHUNK_NUM 14144
#define CHUNK_RAD 15

GLFWwindow* gg_window = 0;
VulkanRenderContext* gg_vrc = 0;
ChunkThreadFreeInfo* gg_freeInfo = 0;

inline uint8_t getVid(uint16_t num) {
	return (uint8_t)(num & 0b0000000000111111);
}

inline uint16_t getType(uint16_t num) {
	return (uint16_t)((num & 0b1111111111000000) >> 6);
}

inline uint16_t setVid(uint16_t num, uint8_t vid) {
	return (num & 0b1111111111000000) +
		(((uint16_t)vid) & 0b0000000000111111);
}

inline uint16_t setType(uint16_t num, uint16_t type) {
	return (num & 0b0000000000111111) +
		((type & 0b0000001111111111) << 6);
}

Vec3i** Chunk::texts = 0;
FastNoise* Chunk::heightNoise = 0;
FastNoise* Chunk::caveNoise = 0;
FastNoise* Chunk::oreNoise = 0;
int Chunk::m_fence = 0;
std::vector<Chunk*> Chunk::chunks = std::vector<Chunk*>();
std::thread* Chunk::chunkThread = 0;
Cube* Chunk::rcubes = 0;
uint32_t Chunk::rcubesSize = 0;
uint32_t Chunk::rcubestSize = 0;
int Chunk::rsize = 0;
uint32_t Chunk::cubeNum = 0;
ChunkModelInfo* Chunk::model = 0;
VulkanRenderContext* Chunk::renderContext = 0;
ChunkThreadFreeInfo* Chunk::freeInfo = 0;
CubeDataNode* Chunk::dataNode = NULL;

inline void* malloc_c(size_t size) {
	void* result = malloc(size);
	memset(result, 0, size);
	return result;
}

void recalcChunksNextTo(Chunk* chunk) {
	chunk->findChunks();
	chunk->recalcqrid();

	if (chunk->m_xn != NULL) {
		chunk->m_xn->findChunks();
		chunk->m_xn->recalcqrid();
	}

	if (chunk->m_xp != NULL) {
		chunk->m_xp->findChunks();
		chunk->m_xp->recalcqrid();
	}

	if (chunk->m_yn != NULL) {
		chunk->m_yn->findChunks();
		chunk->m_yn->recalcqrid();
	}

	if (chunk->m_yp != NULL) {
		chunk->m_yp->findChunks();
		chunk->m_yp->recalcqrid();
	}

	if (chunk->m_zn != NULL) {
		chunk->m_zn->findChunks();
		chunk->m_zn->recalcqrid();
	}

	if (chunk->m_zp != NULL) {
		chunk->m_zp->findChunks();
		chunk->m_zp->recalcqrid();
	}
}

static bool closeEnough(Vec3i pos, Vec3i other) {
	return pos.dist(other) <= CHUNK_RAD;
}

int addNext(Chunk* chunk, Vec3i pos) {
	int result = 0;

	if (chunk->m_xn == NULL) {
		Chunk* temp = new Chunk(pos.add(Vec3i( -1, 0, 0 )));
		recalcChunksNextTo(temp);
		result = 1;
	}

	if (chunk->m_xp == NULL) {
		Chunk* temp = new Chunk(pos.add(Vec3i( 1, 0, 0 )));
		recalcChunksNextTo(temp);
		result = 1;
	}

	if (chunk->m_yn == NULL) {
		Chunk* temp = new Chunk(pos.add(Vec3i( 0, -1, 0 )));
		recalcChunksNextTo(temp);
		result = 1;
	}

	if (chunk->m_yp == NULL) {
		Chunk* temp = new Chunk(pos.add(Vec3i( 0,  1, 0 )));
		recalcChunksNextTo(temp);
		result = 1;
	}

	if (chunk->m_zn == NULL) {
		Chunk* temp = new Chunk(pos.add(Vec3i( 0, 0, -1 )));
		recalcChunksNextTo(temp);
		result = 1;
	}

	if (chunk->m_zp == NULL) {
		Chunk* temp = new Chunk(pos.add(Vec3i( 0, 0,  1 )));
		recalcChunksNextTo(temp);
		result = 1;
	}

	return result;
}

void chunkThreadRun() {
	GLFWwindow* window = gg_window;
	VulkanRenderContext* vrc = gg_vrc;
	ChunkThreadFreeInfo* freeInfo = gg_freeInfo;
	
	Chunk** closeChunks = new Chunk*[CHUNK_NUM];

	for (int i = 0; i < CHUNK_NUM; i++) {
		closeChunks[i] = NULL;
	}

	Cube* rCubesPtr = NULL;
	Cube* pCubesPtr = NULL;

	int prcsz = 0;
	int rcsz = 0;

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = vrc->device->queueIdx;

	VLKCheck(vkCreateCommandPool(vrc->device->device, &commandPoolCreateInfo, NULL, &freeInfo->commandPool),
		"Failed to create command pool");

	VkCommandBufferAllocateInfo transferCommandBufferAllocateInfo = {};
	transferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	transferCommandBufferAllocateInfo.pNext = NULL;
	transferCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	transferCommandBufferAllocateInfo.commandPool = freeInfo->commandPool;
	transferCommandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer transferCmdBuffer;
	VLKCheck(vkAllocateCommandBuffers(vrc->device->device, &transferCommandBufferAllocateInfo, &transferCmdBuffer), 
		"Failed to allocate command buffer");

	VkQueue transferQueue = vrc->device->queue;
	//vkGetDeviceQueue(vrc->device->device, vrc->device->queueIdx, 1, &transferQueue);

	bool nmc = false;
	Vec3i prevPlayerPos = Vec3i(-100, -100, -100);

	while (!glfwWindowShouldClose(window)) {
		Vec3i playerPos = Vec3i( (int)floor(Camera::pos.x / 16.0f), (int)floor(Camera::pos.y / 16.0f), (int)floor(Camera::pos.z / 16.0f) );

		bool moved = false;

		if (playerPos.x != prevPlayerPos.x || playerPos.y != prevPlayerPos.y || playerPos.z != prevPlayerPos.z) {
			nmc = false;
		}

		prevPlayerPos.x = playerPos.x;
		prevPlayerPos.y = playerPos.y;
		prevPlayerPos.z = playerPos.z;

		if (nmc) {
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
			continue;
		}

		Chunk* playerChunk = getChunkAt(playerPos);

		nmc = true;

		if (playerChunk == NULL) {
			Chunk* temp = new Chunk(playerPos);
			recalcChunksNextTo(temp);
			nmc = false;
		} else {
			if (addNext(playerChunk, playerPos)) {
				nmc = false;
			}
		}

		for (int i = 0; i < CHUNK_NUM; i++) {
			closeChunks[i] = NULL;
		}

		int j = 0;
		for (int i = 0; i < Chunk::chunks.size(); i++) {
			if (closeEnough(playerPos, Chunk::chunks[i]->pos)) {
				closeChunks[j] = Chunk::chunks[i];
				j++;
			}
		}

		for (int i = 0; i < CHUNK_NUM; i++) {
			if (closeChunks[i] != NULL && !closeChunks[i]->m_air) {
				if (addNext(closeChunks[i], closeChunks[i]->pos)) {
					nmc = false;
				}
			}
		}

		rcsz = 0;

		for (uint32_t i = 0; i < CHUNK_NUM; i++) {
			Chunk* tcchunk = closeChunks[i];
			if (tcchunk != NULL && !tcchunk->m_air) {
				uint16_t* tccubes = tcchunk->cubes;
				for (uint32_t j = 0; j < 16 * 16 * 16; j++) {
					uint16_t cb = tccubes[j];
					if (getVid(cb)) {
						rcsz = rcsz + 1;
					}
				}
			}
		}

		pCubesPtr = rCubesPtr;
		prcsz = rcsz;

		int rcszct = 0;

		rCubesPtr = new Cube[rcsz];

		for (uint32_t i = 0; i < CHUNK_NUM; i++) {
			Chunk* tcchunk = closeChunks[i];
			if (tcchunk != NULL && !tcchunk->m_air) {
				uint16_t* tccubes = tcchunk->cubes;
				for (uint32_t j = 0; j < 16 * 16 * 16; j++) {
					uint16_t cb = tccubes[j];
					if (getVid(cb)) {
						float z = (float)(j % 16);
						float y = (float)(((j % 256) - z)/16);
						float x = (float)((j - y*16 - z)/256);

						rCubesPtr[rcszct].pos.x = x + tcchunk->m_cubePos.x;
						rCubesPtr[rcszct].pos.y = y + tcchunk->m_cubePos.y;
						rCubesPtr[rcszct].pos.z = z + tcchunk->m_cubePos.z;

						rCubesPtr[rcszct].type = getType(cb);
						rCubesPtr[rcszct].vid = getVid(cb);

						rcszct = rcszct + 1;
					}
				}
			}
		}

		if (rcsz != 0) {

			int rcct = 0;

			for (int i = 0; i < rcsz; i++) {
				if (rCubesPtr[i].vid & 1) {
					rcct = rcct + 6;
				}

				if (rCubesPtr[i].vid & 2) {
					rcct = rcct + 6;
				}

				if (rCubesPtr[i].vid & 4) {
					rcct = rcct + 6;
				}

				if (rCubesPtr[i].vid & 8) {
					rcct = rcct + 6;
				}

				if (rCubesPtr[i].vid & 16) {
					rcct = rcct + 6;
				}

				if (rCubesPtr[i].vid & 32) {
					rcct = rcct + 6;
				}
			}

			freeInfo->modelInfo = (ChunkModelInfo*)malloc(sizeof(ChunkModelInfo));
			freeInfo->modelInfo->start = false;
			freeInfo->modelInfo->model = (VLKModel*)malloc(sizeof(VLKModel));

			VkBufferCreateInfo vertexInputBufferInfo = {};
			vertexInputBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vertexInputBufferInfo.size = rcct * sizeof(Vertex);
			vertexInputBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			vertexInputBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			VLKCheck(vkCreateBuffer(vrc->device->device, &vertexInputBufferInfo, NULL, &freeInfo->modelInfo->model->vertexInputBuffer),
				"Failed to create vertex input buffer.");

			VkMemoryRequirements vertexBufferMemoryRequirements = {};
			vkGetBufferMemoryRequirements(vrc->device->device, freeInfo->modelInfo->model->vertexInputBuffer, &vertexBufferMemoryRequirements);

			VkMemoryAllocateInfo bufferAllocateInfo = {};
			bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			bufferAllocateInfo.allocationSize = vertexBufferMemoryRequirements.size;

			uint32_t vertexMemoryTypeBits = vertexBufferMemoryRequirements.memoryTypeBits;
			VkMemoryPropertyFlags vertexDesiredMemoryFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			for (uint32_t i = 0; i < 32; ++i) {
				VkMemoryType memoryType = vrc->device->memoryProperties.memoryTypes[i];
				if (vertexMemoryTypeBits & 1) {
					if ((memoryType.propertyFlags & vertexDesiredMemoryFlags) == vertexDesiredMemoryFlags) {
						bufferAllocateInfo.memoryTypeIndex = i;
						break;
					}
				}
				vertexMemoryTypeBits = vertexMemoryTypeBits >> 1;
			}

			VLKCheck(vkAllocateMemory(vrc->device->device, &bufferAllocateInfo, NULL, &freeInfo->modelInfo->model->vertexBufferMemory),
				"Failed to allocate buffer memory");

			VLKCheck(vkBindBufferMemory(vrc->device->device, freeInfo->modelInfo->model->vertexInputBuffer, freeInfo->modelInfo->model->vertexBufferMemory, 0),
				"Failed to bind buffer memory");

			vertexInputBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

			VkBuffer srcBuffer;
			VLKCheck(vkCreateBuffer(vrc->device->device, &vertexInputBufferInfo, NULL, &srcBuffer),
				"Failed to create vertex input buffer.");

			vkGetBufferMemoryRequirements(vrc->device->device, srcBuffer, &vertexBufferMemoryRequirements);

			bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			bufferAllocateInfo.allocationSize = vertexBufferMemoryRequirements.size;

			vertexMemoryTypeBits = vertexBufferMemoryRequirements.memoryTypeBits;
			vertexDesiredMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			for (uint32_t i = 0; i < 32; ++i) {
				VkMemoryType memoryType = vrc->device->memoryProperties.memoryTypes[i];
				if (vertexMemoryTypeBits & 1) {
					if ((memoryType.propertyFlags & vertexDesiredMemoryFlags) == vertexDesiredMemoryFlags) {
						bufferAllocateInfo.memoryTypeIndex = i;
						break;
					}
				}
				vertexMemoryTypeBits = vertexMemoryTypeBits >> 1;
			}

			VkDeviceMemory srcMemory;
			VLKCheck(vkAllocateMemory(vrc->device->device, &bufferAllocateInfo, NULL, &srcMemory),
				"Failed to allocate buffer memory");

			void *mapped;
			VLKCheck(vkMapMemory(vrc->device->device, srcMemory, 0, VK_WHOLE_SIZE, 0, &mapped),
				"Failed to map buffer memory");

			Vertex* verts = (Vertex*)mapped;

			int rcp = 0;
			for (int i = 0; i < rcsz; i++) {
				if (rCubesPtr[i].type > 20) {
				}

				Vec3i* tex = Chunk::texts[rCubesPtr[i].type];

				if (rCubesPtr[i].vid & 15) {
					int sx = tex->x % 16;
					int sy = (tex->x - sx) / 16;

					float tex0u = (sx + 0.0) / 16.0;
					float tex0v = (sy + 1.0) / 16.0;

					float tex1u = (sx + 0.0) / 16.0;
					float tex1v = (sy + 0.0) / 16.0;

					float tex2u = (sx + 1.0) / 16.0;
					float tex2v = (sy + 0.0) / 16.0;

					float tex3u = (sx + 1.0) / 16.0;
					float tex3v = (sy + 1.0) / 16.0;

					if (rCubesPtr[i].vid & 1) {
						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 4;
						verts[rcp].u = tex3u;
						verts[rcp].v = tex3v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 2;
						verts[rcp].u = tex1u;
						verts[rcp].v = tex1v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 0;
						verts[rcp].u = tex0u;
						verts[rcp].v = tex0v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 6;
						verts[rcp].u = tex2u;
						verts[rcp].v = tex2v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 2;
						verts[rcp].u = tex1u;
						verts[rcp].v = tex1v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 4;
						verts[rcp].u = tex3u;
						verts[rcp].v = tex3v;

						rcp = rcp + 1;
					}

					if (rCubesPtr[i].vid & 2) {
						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 1;
						verts[rcp].u = tex0u;
						verts[rcp].v = tex0v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 3;
						verts[rcp].u = tex1u;
						verts[rcp].v = tex1v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 5;
						verts[rcp].u = tex3u;
						verts[rcp].v = tex3v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 5;
						verts[rcp].u = tex3u;
						verts[rcp].v = tex3v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 3;
						verts[rcp].u = tex1u;
						verts[rcp].v = tex1v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 7;
						verts[rcp].u = tex2u;
						verts[rcp].v = tex2v;

						rcp = rcp + 1;
					}

					if (rCubesPtr[i].vid & 4) {
						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 5;
						verts[rcp].u = tex3u;
						verts[rcp].v = tex3v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 6;
						verts[rcp].u = tex1u;
						verts[rcp].v = tex1v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 4;
						verts[rcp].u = tex0u;
						verts[rcp].v = tex0v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 7;
						verts[rcp].u = tex2u;
						verts[rcp].v = tex2v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 6;
						verts[rcp].u = tex1u;
						verts[rcp].v = tex1v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 5;
						verts[rcp].u = tex3u;
						verts[rcp].v = tex3v;

						rcp = rcp + 1;
					}

					if (rCubesPtr[i].vid & 8) {
						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 0;
						verts[rcp].u = tex0u;
						verts[rcp].v = tex0v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 2;
						verts[rcp].u = tex1u;
						verts[rcp].v = tex1v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 1;
						verts[rcp].u = tex3u;
						verts[rcp].v = tex3v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 1;
						verts[rcp].u = tex3u;
						verts[rcp].v = tex3v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
						verts[rcp].w = 2;
						verts[rcp].u = tex1u;
						verts[rcp].v = tex1v;

						rcp = rcp + 1;

						verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
						verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
						verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
						verts[rcp].w = 3;
						verts[rcp].u = tex2u;
						verts[rcp].v = tex2v;

						rcp = rcp + 1;
					}
				}

				if (rCubesPtr[i].vid & 16) {
					int sx = tex->y % 16;
					int sy = (tex->y - sx) / 16;

					float tex0u = (sx + 0.0) / 16.0;
					float tex0v = (sy + 1.0) / 16.0;

					float tex1u = (sx + 0.0) / 16.0;
					float tex1v = (sy + 0.0) / 16.0;

					float tex2u = (sx + 1.0) / 16.0;
					float tex2v = (sy + 0.0) / 16.0;

					float tex3u = (sx + 1.0) / 16.0;
					float tex3v = (sy + 1.0) / 16.0;

					verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
					verts[rcp].w = 3;
					verts[rcp].u = tex1u;
					verts[rcp].v = tex1v;

					rcp = rcp + 1;

					verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
					verts[rcp].w = 2;
					verts[rcp].u = tex0u;
					verts[rcp].v = tex0v;

					rcp = rcp + 1;

					verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
					verts[rcp].w = 7;
					verts[rcp].u = tex2u;
					verts[rcp].v = tex2v;

					rcp = rcp + 1;

					verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
					verts[rcp].w = 7;
					verts[rcp].u = tex2u;
					verts[rcp].v = tex2v;

					rcp = rcp + 1;

					verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
					verts[rcp].w = 2;
					verts[rcp].u = tex0u;
					verts[rcp].v = tex0v;

					rcp = rcp + 1;

					verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y + 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
					verts[rcp].w = 6;
					verts[rcp].u = tex3u;
					verts[rcp].v = tex3v;

					rcp = rcp + 1;
				}

				if (rCubesPtr[i].vid & 32) {
					int sx = tex->z % 16;
					int sy = (tex->z - sx) / 16;

					float tex0u = (sx + 0.0) / 16.0;
					float tex0v = (sy + 1.0) / 16.0;

					float tex1u = (sx + 0.0) / 16.0;
					float tex1v = (sy + 0.0) / 16.0;

					float tex2u = (sx + 1.0) / 16.0;
					float tex2v = (sy + 0.0) / 16.0;

					float tex3u = (sx + 1.0) / 16.0;
					float tex3v = (sy + 1.0) / 16.0;

					verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
					verts[rcp].w = 5;
					verts[rcp].u = tex2u;
					verts[rcp].v = tex2v;

					rcp = rcp + 1;

					verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
					verts[rcp].w = 0;
					verts[rcp].u = tex0u;
					verts[rcp].v = tex0v;

					rcp = rcp + 1;

					verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
					verts[rcp].w = 1;
					verts[rcp].u = tex1u;
					verts[rcp].v = tex1v;

					rcp = rcp + 1;

					verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
					verts[rcp].w = 4;
					verts[rcp].u = tex3u;
					verts[rcp].v = tex3v;

					rcp = rcp + 1;

					verts[rcp].x = rCubesPtr[i].pos.x - 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z - 0.5f;
					verts[rcp].w = 0;
					verts[rcp].u = tex0u;
					verts[rcp].v = tex0v;

					rcp = rcp + 1;

					verts[rcp].x = rCubesPtr[i].pos.x + 0.5f;
					verts[rcp].y = rCubesPtr[i].pos.y - 0.5f;
					verts[rcp].z = rCubesPtr[i].pos.z + 0.5f;
					verts[rcp].w = 5;
					verts[rcp].u = tex2u;
					verts[rcp].v = tex2v;

					rcp = rcp + 1;
				}
			}

			vkUnmapMemory(vrc->device->device, srcMemory);

			VLKCheck(vkBindBufferMemory(vrc->device->device, srcBuffer, srcMemory, 0),
				"Failed to bind buffer memory");

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			vkBeginCommandBuffer(transferCmdBuffer, &beginInfo);

			VkBufferCopy bufferCopy = {};
			bufferCopy.dstOffset = 0;
			bufferCopy.srcOffset = 0;
			bufferCopy.size = rcct * sizeof(Vertex);

			vkCmdCopyBuffer(transferCmdBuffer, srcBuffer, 
				freeInfo->modelInfo->model->vertexInputBuffer, 1, &bufferCopy);

			vkEndCommandBuffer(transferCmdBuffer);

			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &transferCmdBuffer;
			VLKCheck(vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE),
				"Could not submit Queue");

			vkQueueWaitIdle(transferQueue);

			vkFreeMemory(vrc->device->device, srcMemory, NULL);
			vkDestroyBuffer(vrc->device->device, srcBuffer, NULL);

			while (Chunk::m_fence > 0) {
				if (glfwWindowShouldClose(window)) {
					return;
				}

				std::this_thread::sleep_for(std::chrono::microseconds(10));
			}
			Chunk::m_fence = 1;

			Chunk::model = freeInfo->modelInfo;
			Chunk::rcubesSize = rcsz;
			Chunk::rcubestSize = rcct;
			Chunk::rcubes = rCubesPtr;

			Chunk::m_fence = 0;
		}

		if (freeInfo->pmodelInfo != NULL) {
			while (!freeInfo->modelInfo->start) {
				if (glfwWindowShouldClose(window)) {
					return;
				}

				std::this_thread::sleep_for(std::chrono::microseconds(10));
			}

			vlkDestroyModel(vrc->device, freeInfo->pmodelInfo->model);

			free(freeInfo->pmodelInfo);
			freeInfo->pmodelInfo = NULL;
		}

		freeInfo->pmodelInfo = freeInfo->modelInfo;

		if (pCubesPtr != NULL) {
			delete[] pCubesPtr;
			pCubesPtr = NULL;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}

void Chunk::init(unsigned int seed, GLFWwindow* window, VulkanRenderContext* vulkanRenderContext) {
	texts = new Vec3i*[6];

	texts[0] = (Vec3i*)malloc(sizeof(Vec3i));
	texts[0]->x = 0;
	texts[0]->y = 0;
	texts[0]->z = 0;

	texts[1] = (Vec3i*)malloc(sizeof(Vec3i));
	texts[1]->x = 3;
	texts[1]->y = 0;
	texts[1]->z = 2;

	texts[2] = (Vec3i*)malloc(sizeof(Vec3i));
	texts[2]->x = 2;
	texts[2]->y = 2;
	texts[2]->z = 2;

	texts[3] = (Vec3i*)malloc(sizeof(Vec3i));
	texts[3]->x = 1;
	texts[3]->y = 1;
	texts[3]->z = 1;

	texts[4] = (Vec3i*)malloc(sizeof(Vec3i));
	texts[4]->x = 34;
	texts[4]->y = 34;
	texts[4]->z = 34;

	texts[5] = (Vec3i*)malloc(sizeof(Vec3i));
	texts[5]->x = 33;
	texts[5]->y = 33;
	texts[5]->z = 33;

	heightNoise = new FastNoise(seed);
	caveNoise = new FastNoise(seed);
	caveNoise->SetFrequency(0.02);
	caveNoise->SetCellularDistanceFunction(FastNoise::Euclidean);
	caveNoise->SetCellularReturnType(FastNoise::Distance2Add);
	oreNoise = new FastNoise(seed);

	rcubes = new Cube[0];
	rcubesSize = 0;
	
	model = NULL;

	renderContext = vulkanRenderContext;

	freeInfo = (ChunkThreadFreeInfo*)malloc(sizeof(ChunkThreadFreeInfo));
	freeInfo->modelInfo = NULL;
	freeInfo->pmodelInfo = NULL;

	dataNode = (CubeDataNode*)malloc_c(sizeof(CubeDataNode));

	dataNode->len = 0;
	dataNode->offset = 0;
	dataNode->data = NULL;
	
	gg_window = window;
	gg_vrc = renderContext;
	gg_freeInfo = freeInfo;

	chunkThread = new std::thread(chunkThreadRun);
}

void Chunk::destroy(VLKDevice* device) {
	chunkThread->join();

	vkDestroyCommandPool(device->device, freeInfo->commandPool, NULL);

	if (freeInfo->pmodelInfo == freeInfo->modelInfo) {
		if (freeInfo->modelInfo != NULL) {
			vlkDestroyModel(device, freeInfo->modelInfo->model);
			free(freeInfo->modelInfo);
			freeInfo->modelInfo = NULL;
		}
	} else {
		if (freeInfo->pmodelInfo != NULL) {
			vlkDestroyModel(device, freeInfo->pmodelInfo->model);
			free(freeInfo->pmodelInfo);
			freeInfo->pmodelInfo = NULL;
		}

		if (freeInfo->modelInfo != NULL) {
			vlkDestroyModel(device, freeInfo->modelInfo->model);
			free(freeInfo->modelInfo);
			freeInfo->modelInfo = NULL;
		}
	}
}

void Chunk::render(VLKDevice* device, VLKSwapchain* swapChain) {
	vlkUniforms(device, renderContext->shader, renderContext->uniformBuffer, sizeof(CubeUniformBuffer));

	while (m_fence == 1) {
		std::this_thread::sleep_for(std::chrono::microseconds(10));
	}
	m_fence = m_fence + 2;

	if (model != NULL) {
		VkViewport viewport = { 0, 0, swapChain->width * 2, swapChain->height * 2, 0, 1 };
		VkRect2D scissor = { {0, 0}, {swapChain->width * 2, swapChain->height * 2} };
		VkDeviceSize offsets = 0;
		
		vkCmdSetViewport(device->drawCmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(device->drawCmdBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderContext->pipeline->pipeline);
		vkCmdBindVertexBuffers(device->drawCmdBuffer, 0, 1, &model->model->vertexInputBuffer, &offsets);
		vkCmdBindDescriptorSets(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			renderContext->pipeline->pipelineLayout, 0, 1, &renderContext->shader->descriptorSet, 0, NULL);

		vkCmdDraw(device->drawCmdBuffer, rcubestSize, 1, 0, 0);
		model->start = true;
	}

	m_fence = m_fence - 2;
}

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

Chunk::Chunk(Vec3i a_pos) {
	pos = Vec3i(a_pos.x, a_pos.y, a_pos.z);
	m_cubePos = Vec3i(a_pos.x * 16, a_pos.y * 16, a_pos.z * 16);

	cubes = new uint16_t[16 * 16 * 16];

	CubeNoise* cubeNoises = new CubeNoise[16 * 16 * 16];

	for (uint8_t x = 0; x < 16; x += 3) {
		float xf = m_cubePos.x + x;
		for (uint8_t z = 0; z < 16; z += 3) {
			float zf = m_cubePos.z + z;
			float nz1 = heightNoise->GetPerlin(xf / 1.2f, 0.8f, zf / 1.2f) * 64;
			float nz2 = heightNoise->GetPerlin(xf * 2, 2.7f, zf * 2) * 16;

			float nz = nz1 + nz2;

			for (uint8_t y = 0; y < 16; y += 3) {
				float yf = m_cubePos.y + y;

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
		float xf = m_cubePos.x + x;
		for (uint8_t z = 0; z < 16; z++) {
			float zf = m_cubePos.z + z;

			for (uint8_t y = 0; y < 16; y++) {
				float yf = m_cubePos.y + y;

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

				cubes[x * 256 + y * 16 + z] = setType(cubes[x * 256 + y * 16 + z], type);
				cubes[x * 256 + y * 16 + z] = setVid(cubes[x * 256 + y * 16 + z], 0);
			}
		}
	}

	delete[] cubeNoises;

	m_air = false;

	Vec3i cp = Vec3i((int)pos.x,(int)pos.y, (int)pos.z);

	putChunkAt(this, cp);

	chunks.push_back(this);
}

void Chunk::findChunks() {
	m_xn = getChunkAt(pos.add(Vec3i( -1, 0, 0 )));
	m_xp = getChunkAt(pos.add(Vec3i(  1, 0, 0 )));
	m_yn = getChunkAt(pos.add(Vec3i( 0, -1, 0 )));
	m_yp = getChunkAt(pos.add(Vec3i( 0,  1, 0 )));
	m_zn = getChunkAt(pos.add(Vec3i( 0, 0, -1 )));
	m_zp = getChunkAt(pos.add(Vec3i( 0, 0,  1 )));
}

void Chunk::recalcqrid() {
	uint8_t tempVid = 0;

	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			for (int z = 0; z < 16; z++) {

				bool xn = x == 0 ?
					(m_xn == NULL ? true : (getType(m_xn->cubes[15 * 256 + y * 16 + z]) == 0))
					: getType(cubes[(x - 1) * 256 + y * 16 + z]) == 0;

				bool xp = x == 15 ?
					(m_xp == NULL ? true : (getType(m_xp->cubes[0 * 256 + y * 16 + z]) == 0))
					: getType(cubes[(x + 1) * 256 + y * 16 + z]) == 0;

				bool yn = y == 0 ?
					(m_yn == NULL ? true : (getType(m_yn->cubes[x * 256 + 15 * 16 + z]) == 0))
					: getType(cubes[x * 256 + (y - 1) * 16 + z]) == 0;

				bool yp = y == 15 ?
					(m_yp == NULL ? true : (getType(m_yp->cubes[x * 256 + 0 * 16 + z]) == 0))
					: getType(cubes[x * 256 + (y + 1) * 16 + z]) == 0;

				bool zn = z == 0 ?
					(m_zn == NULL ? true : (getType(m_zn->cubes[x * 256 + y * 16 + 15]) == 0))
					: getType(cubes[x * 256 + y * 16 + (z - 1)]) == 0;

				bool zp = z == 15 ?
					(m_zp == NULL ? true : (getType(m_zp->cubes[x * 256 + y * 16 + 0]) == 0))
					: getType(cubes[x * 256 + y * 16 + (z + 1)]) == 0;

				tempVid = 0;

				tempVid += (zn ? 1 : 0);
				tempVid += (zp ? 2 : 0);

				tempVid += (xp ? 4 : 0);
				tempVid += (xn ? 8 : 0);

				tempVid += (yp ? 16 : 0);
				tempVid += (yn ? 32 : 0);

				if (!(xn || xp || yn || yp || zn || zp) || getType(cubes[x * 256 + y * 16 + z]) == 0) {
					tempVid = 0;
				}

				cubes[x * 256 + y * 16 + z] = setVid(cubes[x * 256 + y * 16 + z], tempVid);
			}
		}
	}

	m_air = true;
	for (int i = 0; i < 16 * 16 * 16; i++) {
		if (getVid(cubes[i])) {
			m_air = false;
			break;
		}
	}
}

inline Chunk* getChunkAt(Vec3i pos) {
	CubeDataNode* dataNode = Chunk::dataNode;

	if (dataNode == NULL || (pos.x - dataNode->offset) > dataNode->len - 1 || (pos.x - dataNode->offset) < 0) {
		return NULL;
	}

	dataNode = ((CubeDataNode**)dataNode->data)[pos.x - dataNode->offset];

	if (dataNode == NULL || (pos.y - dataNode->offset) > dataNode->len - 1 || (pos.y - dataNode->offset) < 0) {
		return NULL;
	}

	dataNode = ((CubeDataNode**)dataNode->data)[pos.y - dataNode->offset];

	if (dataNode == NULL || (pos.z - dataNode->offset) > dataNode->len - 1 || (pos.z - dataNode->offset) < 0) {
		return NULL;
	}

	return ((Chunk**)dataNode->data)[pos.z - dataNode->offset];
}

void putChunkAt(Chunk* chunk, Vec3i pos) {
	CubeDataNode* dataNode = Chunk::dataNode;

	if (dataNode->len + dataNode->offset - 1 < pos.x) {
		int32_t preLen = dataNode->len;

		dataNode->len = pos.x - dataNode->offset + 1;

		CubeDataNode** preData = (CubeDataNode**)dataNode->data;

		dataNode->data = malloc_c(sizeof(CubeDataNode*) * dataNode->len);

		CubeDataNode** resultData = (CubeDataNode**)dataNode->data;

		for (int i = 0; i < preLen;i++) {
			resultData[i] = preData[i];
		}

		for (int i = preLen; i < dataNode->len;i++) {
			resultData[i] = (CubeDataNode*)malloc_c(sizeof(CubeDataNode));

			resultData[i]->data = NULL;
			resultData[i]->len = 0;
			resultData[i]->offset = 0;
		}

		putChunkAt(chunk, pos);
	} else if (dataNode->offset > pos.x) {
		int32_t preLen = dataNode->len;
		int32_t preOff = dataNode->offset;

		dataNode->offset = pos.x;
		dataNode->len = preLen + preOff - dataNode->offset;

		CubeDataNode** preData = (CubeDataNode**)dataNode->data;

		dataNode->data = malloc_c(sizeof(CubeDataNode*) * dataNode->len);

		CubeDataNode** resultData = (CubeDataNode**)dataNode->data;

		int copyOff = preOff - dataNode->offset;

		for (int i = 0; i < preLen; i++) {
			resultData[i + copyOff] = preData[i];
		}

		for (int i = 0; i < copyOff; i++) {
			resultData[i] = (CubeDataNode*)malloc_c(sizeof(CubeDataNode));

			resultData[i]->data = NULL;
			resultData[i]->len = 0;
			resultData[i]->offset = 0;
		}

		putChunkAt(chunk, pos);
	}

	dataNode = ((CubeDataNode**)dataNode->data)[pos.x - dataNode->offset];

	if (dataNode->len + dataNode->offset - 1 < pos.y) {
		int32_t preLen = dataNode->len;

		dataNode->len = pos.y - dataNode->offset + 1;

		CubeDataNode** preData = (CubeDataNode**)dataNode->data;

		dataNode->data = malloc_c(sizeof(CubeDataNode*) * dataNode->len);

		CubeDataNode** resultData = (CubeDataNode**)dataNode->data;

		for (int i = 0; i < preLen; i++) {
			resultData[i] = preData[i];
		}

		for (int i = preLen; i < dataNode->len; i++) {
			resultData[i] = (CubeDataNode*)malloc_c(sizeof(CubeDataNode));

			resultData[i]->data = NULL;
			resultData[i]->len = 0;
			resultData[i]->offset = 0;
		}

		putChunkAt(chunk, pos);
	}
	else if (dataNode->offset > pos.y) {
		int32_t preLen = dataNode->len;
		int32_t preOff = dataNode->offset;

		dataNode->offset = pos.y;
		dataNode->len = preLen + preOff - dataNode->offset;

		CubeDataNode** preData = (CubeDataNode**)dataNode->data;

		dataNode->data = malloc_c(sizeof(CubeDataNode*) * dataNode->len);

		CubeDataNode** resultData = (CubeDataNode**)dataNode->data;

		int copyOff = preOff - dataNode->offset;

		for (int i = 0; i < preLen; i++) {
			resultData[i + copyOff] = preData[i];
		}

		for (int i = 0; i < copyOff; i++) {
			resultData[i] = (CubeDataNode*)malloc_c(sizeof(CubeDataNode));

			resultData[i]->data = NULL;
			resultData[i]->len = 0;
			resultData[i]->offset = 0;
		}

		putChunkAt(chunk, pos);
	}

	dataNode = ((CubeDataNode**)dataNode->data)[pos.y - dataNode->offset];
	
	if (dataNode->len + dataNode->offset - 1 < pos.z) {
		int32_t preLen = dataNode->len;

		dataNode->len = pos.z - dataNode->offset + 1;

		Chunk** preData = (Chunk**)dataNode->data;

		dataNode->data = malloc_c(sizeof(Chunk*) * dataNode->len);

		Chunk** resultData = (Chunk**)dataNode->data;

		for (int i = 0; i < preLen; i++) {
			resultData[i] = preData[i];
		}

		putChunkAt(chunk, pos);
	}
	else if (dataNode->offset > pos.z) {
		int32_t preLen = dataNode->len;
		int32_t preOff = dataNode->offset;

		dataNode->offset = pos.z;
		dataNode->len = preLen + preOff - dataNode->offset;

		Chunk** preData = (Chunk**)dataNode->data;

		dataNode->data = malloc_c(sizeof(Chunk*) * dataNode->len);

		Chunk** resultData = (Chunk**)dataNode->data;

		int copyOff = preOff - dataNode->offset;

		for (int i = 0; i < preLen; i++) {
			resultData[i + copyOff] = preData[i];
		}

		putChunkAt(chunk, pos);
	}

	((Chunk**)dataNode->data)[pos.z - dataNode->offset] = chunk;
}
