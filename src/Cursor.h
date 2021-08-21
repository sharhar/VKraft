//
//  Cursor.h
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#ifndef Cursor_h
#define Cursor_h

#include <VKL/VKL.h>

class Cursor {
public:
	Cursor(const VKLDevice* device, VKLRenderPass* renderPass, VKLQueue* queue);
	void updateProjection(int width, int height);
	void render(VKLCommandBuffer* cmdBuffer);
	void destroy();
private:
	const VKLDevice* m_device;
	VKLQueue* m_queue;
	
	VKLBuffer m_vertBuffer;
	VKLShader m_shader;
	
	VKLPipeline m_pipeline;
	
	float m_screenSize[2];
	
	VkDescriptorPool m_pool;
	VkDescriptorSet m_descSet;
	VkSampler m_sampler;
	
	VKLImage m_tempImage;
	
	
	//VKLBuffer* m_uniformBuffer;
	
	//VKLUniformObject* m_uniform;
	
};

#endif /* Cursor_h */
