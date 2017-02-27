#include "master.h"
#include <iostream>
#include <assert.h>

#define CHUNK_NUM 922
#define CHUNK_RAD 6

Vec3i** Cube::texts = 0;

void Cube::init() {
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
}

Cube::Cube(Vec3 pos, unsigned int t) {
	m_pos = pos;
	type = t;
	visible = false;

	tex = texts[t];
	vid = 0;
}

Cube::Cube(Cube* other) {
	m_pos = other->m_pos;
	type = other->type;
	visible = other->visible;
	tex = other->tex;
	vid = other->vid;
}

PerlinNoise* Chunk::noise = 0;
int Chunk::m_fence = 0;
std::vector<Chunk*> Chunk::chunks = std::vector<Chunk*>();
std::thread* Chunk::chunkThread = 0;
Cube** Chunk::rcubes = 0;
int Chunk::rcubesSize = 0;
int Chunk::rsize = 0;
uint32_t Chunk::cubeNum = 0;
ChunkModelInfo* Chunk::model = 0;
VulkanRenderContext* Chunk::renderContext = 0;

Chunk* Chunk::getChunkAt(Vec3 pos) {
	int sz = chunks.size();

	for (int i = 0; i < sz; i++) {
		if (chunks[i]->m_pos.x == pos.x && chunks[i]->m_pos.y == pos.y && chunks[i]->m_pos.z == pos.z) {
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

static void chunkThreadRun(GLFWwindow* window, VulkanRenderContext* vrc) {
	Chunk** closeChunks = new Chunk*[CHUNK_NUM];

	for (int i = 0; i < CHUNK_NUM; i++) {
		closeChunks[i] = NULL;
	}

	std::vector<Cube*> renderCubes;
	Cube** rCubesPtr = NULL;
	Cube** pCubesPtr = NULL;

	int prcsz = 0;
	int rcsz = 0;

	ChunkModelInfo* pmodelInfo = NULL;
	ChunkModelInfo* modelInfo = NULL;

	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = vrc->device.presentQueueIdx;

	VkCommandPool commandPool;
	VLKCheck(vkCreateCommandPool(vrc->device.device, &commandPoolCreateInfo, NULL, &commandPool),
		"Failed to create command pool");

	VkCommandBufferAllocateInfo commandBufferAllocationInfo = {};
	commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocationInfo.commandPool = commandPool;
	commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	commandBufferAllocationInfo.commandBufferCount = 2 * vrc->swapChain.imageCount;

	VkCommandBuffer* commandBuffer = new VkCommandBuffer[2 * vrc->swapChain.imageCount];
	VLKCheck(vkAllocateCommandBuffers(vrc->device.device, &commandBufferAllocationInfo, commandBuffer),
		"Failed to allocate command buffers");

	int commandBuferID = 0;

	while (!glfwWindowShouldClose(window)) {
		Vec3 playerPos = { floor(Camera::pos.x / 16.0f), floor(Camera::pos.y / 16.0f), floor(Camera::pos.z / 16.0f) };

		Chunk* playerChunk = Chunk::getChunkAt(playerPos);

		if (playerChunk == NULL) {
			Chunk* temp = new Chunk(playerPos);
			recalcChunksNextTo(temp);
		} else {
			if (playerChunk->m_xn == NULL) {
				Chunk* temp = new Chunk(playerPos.add({ -1, 0, 0 }));
				recalcChunksNextTo(temp);
			}

			if (playerChunk->m_xp == NULL) {
				Chunk* temp = new Chunk(playerPos.add({ 1, 0, 0 }));
				recalcChunksNextTo(temp);
			}

			if (playerChunk->m_yn == NULL) {
				Chunk* temp = new Chunk(playerPos.add({ 0, -1, 0 }));
				recalcChunksNextTo(temp);
			}

			if (playerChunk->m_yn == NULL) {
				Chunk* temp = new Chunk(playerPos.add({ 0,  1, 0 }));
				recalcChunksNextTo(temp);
			}

			if (playerChunk->m_zn == NULL) {
				Chunk* temp = new Chunk(playerPos.add({ 0, 0, -1 }));
				recalcChunksNextTo(temp);
			}

			if (playerChunk->m_zp == NULL) {
				Chunk* temp = new Chunk(playerPos.add({ 0, 0,  1 }));
				recalcChunksNextTo(temp);
			}
		}

		for (int i = 0; i < CHUNK_NUM; i++) {
			closeChunks[i] = NULL;
		}

		int j = 0;
		for (int i = 0; i < Chunk::chunks.size(); i++) {
			if (closeEnough(playerPos, Chunk::chunks[i]->m_pos)) {
				closeChunks[j] = Chunk::chunks[i];
				j++;
			}
		}

		for (int i = 0; i < CHUNK_NUM; i++) {
			if (closeChunks[i] != NULL && !closeChunks[i]->m_air) {
				if (closeChunks[i]->m_xn == NULL) {
					Chunk* temp = new Chunk(closeChunks[i]->m_pos.add({ -1, 0, 0 }));
					recalcChunksNextTo(temp);
				}

				if (closeChunks[i]->m_xp == NULL) {
					Chunk* temp = new Chunk(closeChunks[i]->m_pos.add({ 1, 0, 0 }));
					recalcChunksNextTo(temp);
				}

				if (closeChunks[i]->m_yn == NULL) {
					Chunk* temp = new Chunk(closeChunks[i]->m_pos.add({ 0, -1, 0 }));
					recalcChunksNextTo(temp);
				}

				if (closeChunks[i]->m_yn == NULL) {
					Chunk* temp = new Chunk(closeChunks[i]->m_pos.add({ 0,  1, 0 }));
					recalcChunksNextTo(temp);
				}

				if (closeChunks[i]->m_zn == NULL) {
					Chunk* temp = new Chunk(closeChunks[i]->m_pos.add({ 0, 0, -1 }));
					recalcChunksNextTo(temp);
				}

				if (closeChunks[i]->m_zp == NULL) {
					Chunk* temp = new Chunk(closeChunks[i]->m_pos.add({ 0, 0,  1 }));
					recalcChunksNextTo(temp);
				}
			}
		}

		renderCubes.clear();
		for (int i = 0; i < CHUNK_NUM; i++) {
			Chunk* tcchunk = closeChunks[i];

			if (tcchunk != NULL && !tcchunk->m_air) {
				Cube** tccubes = tcchunk->cubes;
				for (int j = 0; j < 16 * 16 * 16; j++) {
					Cube* cb = tccubes[j];
					if (cb != NULL && cb->visible) {
						renderCubes.push_back(cb);
					}
				}
			}
		}

		pCubesPtr = rCubesPtr;
		prcsz = rcsz;

		rcsz = renderCubes.size();
		rCubesPtr = new Cube*[rcsz];

		for (int i = 0; i < rcsz; i++) {
			rCubesPtr[i] = new Cube(renderCubes[i]);
		}

		if (rcsz != 0) {
			Vertex* verts = new Vertex[rcsz];
			for (int i = 0; i < rcsz; i++) {
				verts[i].x = rCubesPtr[i]->m_pos.x;
				verts[i].y = 0.5f - rCubesPtr[i]->m_pos.y;
				verts[i].z = rCubesPtr[i]->m_pos.z;


				verts[i].tx = rCubesPtr[i]->tex->x;
				verts[i].ty = rCubesPtr[i]->tex->y;
				verts[i].tz = rCubesPtr[i]->tex->z;
				verts[i].rinf = rCubesPtr[i]->vid;
			}

			modelInfo = (ChunkModelInfo*)malloc(sizeof(ChunkModelInfo));
			modelInfo->full = true;
			modelInfo->model = vlkCreateModel(vrc->device, verts, rcsz);

			VkCommandBufferInheritanceInfo inheritanceInfo = {};
			inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
			inheritanceInfo.pNext = NULL;
			inheritanceInfo.occlusionQueryEnable = VK_FALSE;
			inheritanceInfo.queryFlags = 0;
			inheritanceInfo.pipelineStatistics = 0;
			inheritanceInfo.renderPass = vrc->swapChain.renderPass;
			inheritanceInfo.subpass = 0;

			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
			beginInfo.pInheritanceInfo = &inheritanceInfo;

			VkViewport viewport = { 0, 0, vrc->swapChain.width, vrc->swapChain.height, 0, 1 };
			VkRect2D scissor = { 0, 0, vrc->swapChain.width, vrc->swapChain.height };
			VkDeviceSize offsets = {};

			modelInfo->commandBuffer = new VkCommandBuffer[2 * vrc->swapChain.imageCount];

			for (int i = 0; i < vrc->swapChain.imageCount;i++) {
				inheritanceInfo.framebuffer = vrc->swapChain.frameBuffers[i];

				vkBeginCommandBuffer(commandBuffer[commandBuferID * 2 + i], &beginInfo);

				vkCmdSetViewport(commandBuffer[commandBuferID * 2 + i], 0, 1, &viewport);
				vkCmdSetScissor(commandBuffer[commandBuferID * 2 + i], 0, 1, &scissor);
				vkCmdBindPipeline(commandBuffer[commandBuferID * 2 + i], VK_PIPELINE_BIND_POINT_GRAPHICS, vrc->pipeline.pipeline);
				vkCmdBindVertexBuffers(commandBuffer[commandBuferID * 2 + i], 0, 1, &modelInfo->model.vertexInputBuffer, &offsets);
				vkCmdBindDescriptorSets(commandBuffer[commandBuferID * 2 + i], VK_PIPELINE_BIND_POINT_GRAPHICS,
					vrc->pipeline.pipelineLayout, 0, 1, &vrc->shader.descriptorSet, 0, NULL);

				vkCmdDraw(commandBuffer[commandBuferID * 2 + i], rcsz, 1, 0, 0);

				vkEndCommandBuffer(commandBuffer[commandBuferID * 2 + i]);

				modelInfo->commandBuffer[i] = commandBuffer[commandBuferID * 2 + i];
			}

			if (commandBuferID == 0) {
				commandBuferID = 1;
			} else {
				commandBuferID == 0;
			}

			delete[] verts;
		}

		while (Chunk::m_fence > 0) {
			std::this_thread::sleep_for(std::chrono::microseconds(10));
		}
		Chunk::m_fence = 1;

		Chunk::model = modelInfo;
		Chunk::rcubesSize = rcsz;
		Chunk::rcubes = rCubesPtr;
		
		Chunk::m_fence = 0;

		if (pmodelInfo != NULL) {
			vlkDestroyModel(vrc->device, pmodelInfo->model);
			
			free(pmodelInfo);
		}

		pmodelInfo = modelInfo;

		if (pCubesPtr != NULL) {
			for (int i = 0; i < prcsz; i++) {
				delete pCubesPtr[i];
			}

			delete[] pCubesPtr;
			pCubesPtr = NULL;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}

void Chunk::init(unsigned int seed, GLFWwindow* window, VulkanRenderContext* vulkanRenderContext) {
	noise = new PerlinNoise(seed);

	rcubes = new Cube*[0];
	rcubesSize = 0;
	
	model = NULL;

	renderContext = vulkanRenderContext;

	chunkThread = new std::thread(chunkThreadRun, window, renderContext);
}

void Chunk::destroy(VLKDevice device) {
	chunkThread->join();

	if (model->full) {
		vlkDestroyModel(device, model->model);
	}
}

void Chunk::render(VLKDevice& device, VLKSwapchain& swapChain) {
	vlkUniforms(device, renderContext->shader, renderContext->uniformBuffer, sizeof(CubeUniformBuffer));

	while (m_fence == 1) {
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	m_fence = m_fence + 2;

	if (model != NULL) {
		vkCmdExecuteCommands(device.drawCmdBuffer, 1, &model->commandBuffer[swapChain.nextImageIdx]);
	}

	m_fence = m_fence - 2;
}

Chunk::Chunk(Vec3 pos) {
	m_pos = { pos.x, pos.y, pos.z };
	m_cubePos = { pos.x * 16, pos.y * 16, pos.z * 16 };

	cubes = new Cube*[16 * 16 * 16];

	for (int x = 0; x < 16; x++) {
		float xf = m_cubePos.x + x;
		for (int z = 0; z < 16; z++) {
			float zf = m_cubePos.z + z;
			float nz = (noise->noise(xf / 25.0f, 0.8f, zf / 25.0f) - 0.5) * 16;

			for (int y = 0; y < 16; y++) {
				float yf = m_cubePos.y + y;

				float yh = yf - nz;

				int type = 0;

				if (yh <= 0) {
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

					float nzc = noise->noise(xf / 25.0f, yf / 25.0f, zf / 25.0f);

					if (nzc > 0.7) {
						type = 0;
					}


				}

				cubes[x * 256 + y * 16 + z] = new Cube({ xf, yf, zf }, type);
			}
		}
	}

	m_air = false;

	chunks.push_back(this);
}

void Chunk::findChunks() {
	m_xn = getChunkAt(m_pos.add({ -1, 0, 0 }));
	m_xp = getChunkAt(m_pos.add({ 1, 0, 0 }));
	m_yn = getChunkAt(m_pos.add({ 0, -1, 0 }));
	m_yp = getChunkAt(m_pos.add({ 0,  1, 0 }));
	m_zn = getChunkAt(m_pos.add({ 0, 0, -1 }));
	m_zp = getChunkAt(m_pos.add({ 0, 0,  1 }));
}

void Chunk::recalcqrid() {
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			for (int z = 0; z < 16; z++) {

				bool xn = x == 0 ?
					(m_xn == NULL ? true : (m_xn->cubes[15 * 256 + y * 16 + z]->type == 0))
					: cubes[(x - 1) * 256 + y * 16 + z]->type == 0;

				bool xp = x == 15 ?
					(m_xp == NULL ? true : (m_xp->cubes[0 * 256 + y * 16 + z]->type == 0))
					: cubes[(x + 1) * 256 + y * 16 + z]->type == 0;

				bool yn = y == 0 ?
					(m_yn == NULL ? true : (m_yn->cubes[x * 256 + 15 * 16 + z]->type == 0))
					: cubes[x * 256 + (y - 1) * 16 + z]->type == 0;

				bool yp = y == 15 ?
					(m_yp == NULL ? true : (m_yp->cubes[x * 256 + 0 * 16 + z]->type == 0))
					: cubes[x * 256 + (y + 1) * 16 + z]->type == 0;

				bool zn = z == 0 ?
					(m_zn == NULL ? true : (m_zn->cubes[x * 256 + y * 16 + 15]->type == 0))
					: cubes[x * 256 + y * 16 + (z - 1)]->type == 0;

				bool zp = z == 15 ?
					(m_zp == NULL ? true : (m_zp->cubes[x * 256 + y * 16 + 0]->type == 0))
					: cubes[x * 256 + y * 16 + (z + 1)]->type == 0;

				cubes[x * 256 + y * 16 + z]->vid = 0;

				cubes[x * 256 + y * 16 + z]->vid += (zn ? 1 : 0);
				cubes[x * 256 + y * 16 + z]->vid += (zp ? 2 : 0);

				cubes[x * 256 + y * 16 + z]->vid += (xp ? 4 : 0);
				cubes[x * 256 + y * 16 + z]->vid += (xn ? 8 : 0);

				cubes[x * 256 + y * 16 + z]->vid += (yp ? 16 : 0);
				cubes[x * 256 + y * 16 + z]->vid += (yn ? 32 : 0);

				if ((xn || xp || yn || yp || zn || zp) && cubes[x * 256 + y * 16 + z]->type != 0) {
					cubes[x * 256 + y * 16 + z]->visible = true;
				}
				else {
					cubes[x * 256 + y * 16 + z]->visible = false;
				}
			}
		}
	}

	m_air = true;
	for (int i = 0; i < 16 * 16 * 16; i++) {
		if (cubes[i]->visible) {
			m_air = false;
			break;
		}
	}
}