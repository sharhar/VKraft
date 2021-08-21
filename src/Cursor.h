#ifndef Cursor_h
#define Cursor_h

#include <VKL/VKL.h>

class Cursor {
public:
	Cursor(const VKLDevice* device, VKLRenderPass* renderPass, VKLQueue* queue, VkImageView view);
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
	
	VKLDescriptorSet* m_descriptorSet;
	
};

#endif /* Cursor_h */
