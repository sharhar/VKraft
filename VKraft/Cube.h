#pragma once

#include "FastNoise.h"
#include "VLKUtils.h"
#include "Utils.h"
#include <thread>

inline uint8_t getVid(uint16_t num);
inline uint16_t getType(uint16_t num);
inline uint16_t setVid(uint16_t num, uint8_t vid);
inline uint16_t setType(uint16_t num, uint16_t type);

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
	static FastNoise* heightNoise;
	static FastNoise* caveNoise;
	static FastNoise* oreNoise;
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
	static uint32_t rcubesSize;
	static uint32_t rcubestSize;
	static Cube* rcubes;
	static int rsize;
	static Vec3i** texts;

	static uint32_t cubeNum;
	static ChunkModelInfo* model;
	static ChunkThreadFreeInfo* freeInfo;
	static VulkanRenderContext* renderContext;

	uint16_t* cubes;

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