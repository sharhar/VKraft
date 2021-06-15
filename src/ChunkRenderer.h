//
//  ChunkManager.h
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#ifndef ChunkRenderer_h
#define ChunkRenderer_h

#include <VKL/VKL.h>
#include "Utils.h"
#include "Chunk.h"

#include <thread>
#include <vector>

typedef struct ChunkUniform {
	float view[16] = { 1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 };

	float proj[16] = { 1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1 };

	Vec3 selected = Vec3( 0.5f, 0.5f, 0.5f );

	float density = 1.0f / 200.0f;
	float gradient = 25;
} ChunkUniform;

class ChunkRenderer {
public:
	static void init(VKLDevice* device, VKLFrameBuffer* framebuffer);
	static void rebuildPipeline();
	static void render(VkCommandBuffer cmdBuffer);
	static ChunkUniform* getUniform();
	static void destroy();
	
	static void addChunk(Vec3i pos);
private:
	static std::vector<Chunk> m_chunks;
	static ChunkUniform* m_chunkUniformBufferData;
	
	static VKLDevice* m_device;
	static VKLFrameBuffer* m_framebuffer;
	static VKLBuffer* m_uniformBuffer;
	static VKLBuffer* m_vertBuffer;
	static VKLShader* m_shader;
	static VKLUniformObject* m_uniform;
	static VKLPipeline* m_pipeline;
	static VKLTexture* m_texture;
};

#endif /* ChunkRenderer_h */
