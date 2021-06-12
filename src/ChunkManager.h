//
//  ChunkManager.hpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#ifndef ChunkManager_h
#define ChunkManager_h

#include <VKL/VKL.h>
#include "Utils.h"

#include <thread>

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

class ChunkManager {
public:
	static void init(VKLDevice* device, int width, int height);
	static void render(VkCommandBuffer cmdBuffer);
	static VKLFrameBuffer* getFramebuffer();
	static void destroy();
private:
	static void internalThreadFunction();
	
	static std::thread* m_thread;
	static ChunkUniform* m_chunkUniformBufferData;
	
	static VKLDevice* m_device;
	static VKLFrameBuffer* m_framebuffer;
	static VKLBuffer* m_uniformBuffer;
	static VKLShader* m_shader;
	static VKLUniformObject* m_uniform;
	static VKLPipeline* m_pipeline;
	static VKLTexture* m_texture;
};

#endif /* ChunkManager_hpp */
