//
//  ChunkManager.h
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#ifndef ChunkRenderer_h
#define ChunkRenderer_h

#include "Base.h"
#include "Utils.h"
#include "Chunk.h"

#include "Texture.h"

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

	MathUtils::Vec3 selected = MathUtils::Vec3( 0.5f, 0.5f, 0.5f );

	float density = 1.0f / 200.0f;
	float gradient = 25;
} ChunkUniform;

class ChunkRenderer {
public:
	ChunkRenderer(Application* application);
	void rebuildPipeline();
	void render(const VKLCommandBuffer* cmdBuffer);
	ChunkUniform* getUniform();
	void destroy();
private:
	ChunkUniform* m_chunkUniformBufferData;
	Application* m_application;
	
	const VKLDevice* m_device;
	const VKLFramebuffer* m_framebuffer;
	
	VKLBuffer m_vertBuffer;
	VKLPipelineLayout m_layout;
	VKLPipeline m_pipeline;
	
	Texture* m_texture;
	
	VKLDescriptorSet* m_descriptorSet;
};

#endif /* ChunkRenderer_h */
