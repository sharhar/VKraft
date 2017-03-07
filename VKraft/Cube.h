#pragma once

#include "PerlinNoise.h"
#include "VLKUtils.h"
#include "Utils.h"
#include <thread>

inline uint8_t getVid(uint32_t num) {
	return (uint8_t)(num & 0b00000000000000000000000000111111);
}

inline Vec3i8 getPos(uint32_t num) {
	uint8_t x = (uint8_t)((num & 0b00000000000000000000001111000000) >> 6);
	uint8_t y = (uint8_t)((num & 0b00000000000000000011110000000000) >> 10);
	uint8_t z = (uint8_t)((num & 0b00000000000000111100000000000000) >> 14);

	return{ x, y, z };
}

inline uint16_t getType(uint32_t num) {
	return (uint16_t)((num & 0b11111111111111000000000000000000) >> 18);
}

inline uint32_t setVid(uint32_t num, uint8_t vid) {
	return (num & 0b11111111111111111111111111000000) +
		(((uint32_t)vid) & 0b00111111);
}

inline uint32_t setPos(uint32_t num, Vec3i8 pos) {
	return (num & 0b11111111111111000000000000111111) +
		((((uint32_t)pos.x) & 0b00001111) << 6) +
		((((uint32_t)pos.y) & 0b00001111) << 10) +
		((((uint32_t)pos.z) & 0b00001111) << 14);
}

inline uint32_t setType(uint32_t num, uint16_t type) {
	return (num & 0b00000000000000111111111111111111) +
		((((uint32_t)type) & 0b0011111111111111) << 18);
}

typedef struct CubeUniformBuffer {
	float view[16] = { 1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 };

	float proj[16] = { 1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 };

	Vec3 selected = { 0, -9, 0 };
} CubeUniformBuffer;

typedef struct ChunkModelInfo {
	VLKModel* model;
	VkCommandBuffer commandBuffer;
	bool start;
} ChunkmodelInfo;

typedef struct VulkanRenderContext {
	VLKDevice* device;
	VLKSwapchain* swapChain;
	VLKFramebuffer* framebuffer;
	VLKShader* shader;
	VLKPipeline* pipeline;
	CubeUniformBuffer* uniformBuffer;
} VulkanRenderContext;

typedef struct ChunkThreadFreeInfo {
	VkCommandPool commandPool;
	ChunkModelInfo* modelInfo;
	ChunkModelInfo* pmodelInfo;
} ChunkThreadFreeInfo;

typedef struct Cube {
	Vec3 pos;
	uint8_t vid;
	uint16_t type;
} Cube;

class Chunk {
private:
	static PerlinNoise* noise;
	static std::thread* chunkThread;
	static std::vector<Chunk*> chunks;

	Vec3 pos;
	Vec3 m_cubePos;

	Chunk* m_xn;
	Chunk* m_xp;
	Chunk* m_yn;
	Chunk* m_yp;
	Chunk* m_zn;
	Chunk* m_zp;

	bool m_air;
public:
	static Chunk* getChunkAt(Vec3 pos);
	static int m_fence;
	static int rcubesSize;
	static Cube* rcubes;
	static int rsize;
	static Vec3i** texts;

	static uint32_t cubeNum;
	static ChunkModelInfo* model;
	static ChunkThreadFreeInfo* freeInfo;
	static VulkanRenderContext* renderContext;

	uint32_t* cubes;

	static void init(unsigned int seed, GLFWwindow* window, VulkanRenderContext* vulkanRenderContext);
	static void destroy(VLKDevice* device);
	static void render(VLKDevice* device, VLKSwapchain* swapChain);

	Chunk(Vec3 pos);

	void recalcqrid();
	void findChunks();

	friend static void chunkThreadRun(GLFWwindow* window, VulkanRenderContext* vulkanRenderContext, ChunkThreadFreeInfo* freeInfo);
	friend static void recalcChunksNextTo(Chunk* chunk);
	friend static void addNext(Chunk* chunk, Vec3 pos);
};