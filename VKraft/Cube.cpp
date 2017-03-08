#include "master.h"
#include <iostream>
#include <assert.h>

#define CHUNK_NUM 4166
#define CHUNK_RAD 10

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
PerlinNoise* Chunk::noise = 0;
int Chunk::m_fence = 0;
std::vector<Chunk*> Chunk::chunks = std::vector<Chunk*>();
std::thread* Chunk::chunkThread = 0;
Cube* Chunk::rcubes = 0;
int Chunk::rcubesSize = 0;
int Chunk::rsize = 0;
uint32_t Chunk::cubeNum = 0;
ChunkModelInfo* Chunk::model = 0;
VulkanRenderContext* Chunk::renderContext = 0;
ChunkThreadFreeInfo* Chunk::freeInfo = 0;

Chunk* Chunk::getChunkAt(Vec3 pos) {
	int sz = chunks.size();

	for (int i = 0; i < sz; i++) {
		if (chunks[i]->pos.x == pos.x && chunks[i]->pos.y == pos.y && chunks[i]->pos.z == pos.z) {
			return chunks[i];
		}
	}

	return NULL;
}

static void recalcChunksNextTo(Chunk* chunk) {
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

static bool closeEnough(Vec3 pos, Vec3 other) {
	return pos.dist(other) <= CHUNK_RAD;
}

static void addNext(Chunk* chunk, Vec3 pos) {
	if (chunk->m_xn == NULL) {
		Chunk* temp = new Chunk(pos.add({ -1, 0, 0 }));
		recalcChunksNextTo(temp);
	}

	if (chunk->m_xp == NULL) {
		Chunk* temp = new Chunk(pos.add({ 1, 0, 0 }));
		recalcChunksNextTo(temp);
	}

	if (chunk->m_yn == NULL) {
		Chunk* temp = new Chunk(pos.add({ 0, -1, 0 }));
		recalcChunksNextTo(temp);
	}

	if (chunk->m_yp == NULL) {
		Chunk* temp = new Chunk(pos.add({ 0,  1, 0 }));
		recalcChunksNextTo(temp);
	}

	if (chunk->m_zn == NULL) {
		Chunk* temp = new Chunk(pos.add({ 0, 0, -1 }));
		recalcChunksNextTo(temp);
	}

	if (chunk->m_zp == NULL) {
		Chunk* temp = new Chunk(pos.add({ 0, 0,  1 }));
		recalcChunksNextTo(temp);
	}
}

static void chunkThreadRun(GLFWwindow* window, VulkanRenderContext* vrc, ChunkThreadFreeInfo* freeInfo) {
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
	commandPoolCreateInfo.queueFamilyIndex = vrc->device->presentQueueIdx;

	VkCommandPool commandPool;
	VLKCheck(vkCreateCommandPool(vrc->device->device, &commandPoolCreateInfo, NULL, &commandPool),
		"Failed to create command pool");

	VkCommandBufferAllocateInfo transferCommandBufferAllocateInfo = {};
	transferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	transferCommandBufferAllocateInfo.pNext = NULL;
	transferCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	transferCommandBufferAllocateInfo.commandPool = commandPool;
	transferCommandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer transferCmdBuffer;
	VLKCheck(vkAllocateCommandBuffers(vrc->device->device, &transferCommandBufferAllocateInfo, &transferCmdBuffer), 
		"Failed to allocate command buffer");

	VkQueue transferQueue;
	vkGetDeviceQueue(vrc->device->device, vrc->device->presentQueueIdx, 2, &transferQueue);

	while (!glfwWindowShouldClose(window)) {
		Vec3 playerPos = { floor(Camera::pos.x / 16.0f), floor(Camera::pos.y / 16.0f), floor(Camera::pos.z / 16.0f) };

		Chunk* playerChunk = Chunk::getChunkAt(playerPos);

		if (playerChunk == NULL) {
			Chunk* temp = new Chunk(playerPos);
			recalcChunksNextTo(temp);
		} else {
			addNext(playerChunk, playerPos);
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
				addNext(closeChunks[i], closeChunks[i]->pos);
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
			freeInfo->modelInfo = (ChunkModelInfo*)malloc(sizeof(ChunkModelInfo));
			freeInfo->modelInfo->start = false;
			freeInfo->modelInfo->model = (VLKModel*)malloc(sizeof(VLKModel));

			VkBufferCreateInfo vertexInputBufferInfo = {};
			vertexInputBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			vertexInputBufferInfo.size = rcsz * sizeof(Vertex);
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

			Vec3i* tex = NULL;

			for (int i = 0; i < rcsz; i++) {
				verts[i].x = rCubesPtr[i].pos.x;
				verts[i].y = rCubesPtr[i].pos.y;
				verts[i].z = rCubesPtr[i].pos.z;

				tex = Chunk::texts[rCubesPtr[i].type];

				verts[i].tx = tex->x;
				verts[i].ty = tex->y;
				verts[i].tz = tex->z;
				verts[i].rinf = rCubesPtr[i].vid;
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
			bufferCopy.size = rcsz * sizeof(Vertex);

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

	noise = new PerlinNoise(seed);

	rcubes = new Cube[0];
	rcubesSize = 0;
	
	model = NULL;

	renderContext = vulkanRenderContext;

	freeInfo = (ChunkThreadFreeInfo*)malloc(sizeof(ChunkThreadFreeInfo));
	freeInfo->modelInfo = NULL;
	freeInfo->pmodelInfo = NULL;

	chunkThread = new std::thread(chunkThreadRun, window, renderContext, freeInfo);
}

void Chunk::destroy(VLKDevice* device) {
	chunkThread->join();

	//vkDestroyCommandPool(device->device, freeInfo->commandPool, NULL);

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
		VkViewport viewport = { 0, 0, swapChain->width, swapChain->height, 0, 1 };
		VkRect2D scissor = { 0, 0, swapChain->width, swapChain->height };
		VkDeviceSize offsets = {};
		
		vkCmdSetViewport(device->drawCmdBuffer, 0, 1, &viewport);
		vkCmdSetScissor(device->drawCmdBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderContext->pipeline->pipeline);
		vkCmdBindVertexBuffers(device->drawCmdBuffer, 0, 1, &model->model->vertexInputBuffer, &offsets);
		vkCmdBindDescriptorSets(device->drawCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			renderContext->pipeline->pipelineLayout, 0, 1, &renderContext->shader->descriptorSet, 0, NULL);

		vkCmdDraw(device->drawCmdBuffer, rcubesSize, 1, 0, 0);
		model->start = true;
	}

	m_fence = m_fence - 2;
}

Chunk::Chunk(Vec3 a_pos) {
	pos = { a_pos.x, a_pos.y, a_pos.z };
	m_cubePos = { a_pos.x * 16, a_pos.y * 16, a_pos.z * 16 };

	cubes = new uint16_t[16 * 16 * 16];

	for (uint8_t x = 0; x < 16; x++) {
		float xf = m_cubePos.x + x;
		for (uint8_t z = 0; z < 16; z++) {
			float zf = m_cubePos.z + z;
			float nz1 = (noise->noise(xf / 100.0f, 0.8f, zf / 100.0f) - 0.5) * 64;
			float nz2 = (noise->noise(xf / 25.0f, 2.7f, zf / 25.0f) - 0.5) * 16;

			float nz = nz1 + nz2;

			for (uint8_t y = 0; y < 16; y++) {
				float yf = m_cubePos.y + y;

				float yh = yf - nz;

				int type = 0;

				if (yh <= 0) {
					float nzc = noise->noise(xf / 25.0f, yf / 25.0f, zf / 25.0f);

					if (nzc < 0.7) {
						if (yh <= 0 && yh > -1) {
							type = 1;
						}
						else if (yh <= -1 && yh > -5) {
							type = 2;
						}
						else {
							float cnzc = noise->noise(xf / 5.0f, yf / 5.0f, zf / 5.0f);
							float inzc = noise->noise(xf / 2.50f, yf / 2.50f, zf / 2.50f);

							if (cnzc > 0.8) {
								type = 4;
							}
							else if (inzc > 0.8) {
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

	m_air = false;

	chunks.push_back(this);
}

void Chunk::findChunks() {
	m_xn = getChunkAt(pos.add({ -1, 0, 0 }));
	m_xp = getChunkAt(pos.add({ 1, 0, 0 }));
	m_yn = getChunkAt(pos.add({ 0, -1, 0 }));
	m_yp = getChunkAt(pos.add({ 0,  1, 0 }));
	m_zn = getChunkAt(pos.add({ 0, 0, -1 }));
	m_zp = getChunkAt(pos.add({ 0, 0,  1 }));
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