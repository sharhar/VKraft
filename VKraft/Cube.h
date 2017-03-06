#pragma once

#include "PerlinNoise.h"
#include "VLKUtils.h"
#include "Utils.h"
#include <thread>

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

class Cube {
public:
	Vec3 m_pos;
	Vec3i* tex;
	int vid;
	unsigned int type;
	bool visible;

	Cube(Vec3 pos, unsigned int t);
	Cube(Cube* other);
};

class Chunk {
private:
	static PerlinNoise* noise;
	static std::thread* chunkThread;
	static std::vector<Chunk*> chunks;

	Vec3 m_pos;
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
	static Cube** rcubes;
	static int rsize;
	static Vec3i** texts;

	static uint32_t cubeNum;
	static ChunkModelInfo* model;
	static ChunkThreadFreeInfo* freeInfo;
	static VulkanRenderContext* renderContext;

	Cube** cubes;

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