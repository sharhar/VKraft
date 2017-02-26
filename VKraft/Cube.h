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
} CubeUniformBuffer;

typedef struct PlayerInfo {
	Vec3 pos;
} PlayerInfo;

typedef struct ChunkModelInfo {
	VLKModel model;
	bool full;
} ChunkmodelInfo;

class Cube {
public:
	static Vec3i** texts;

	Vec3 m_pos;
	Vec3i* tex;
	int vid;
	unsigned int type;
	bool visible;

	Cube(Vec3 pos, unsigned int t);
	Cube(Cube* other);

	static void init();
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

	static uint32_t cubeNum;
	static ChunkModelInfo* model;
	static VLKShader shader;
	static VLKPipeline pipeline;
	static VLKTexture texture;
	static CubeUniformBuffer* cubeUniformBuffer;

	Cube** cubes;

	static void init(unsigned int seed, GLFWwindow* window, VLKDevice device, VLKSwapchain swapChain, CubeUniformBuffer* uniformBuffer, PlayerInfo* playerInfo);
	static void destroy(VLKDevice device);
	static void render(VLKDevice& device, VLKSwapchain& swapChain);

	Chunk(Vec3 pos);

	void recalcqrid();
	void findChunks();

	friend static void chunkThreadRun(GLFWwindow* window, PlayerInfo* playerInfo, VLKDevice* device);
	friend static void recalcChunksNextTo(Chunk* chunk);
};