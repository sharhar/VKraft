//
//  Cursor.hpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#ifndef Cursor_h
#define Cursor_h

#include <VKL/VKL.h>

class Cursor {
public:
	Cursor(VKLDevice* device, VKLSwapChain* swapChain, VKLFrameBuffer* framebuffer);
	
	void updateProjection(int width, int height);
	
	void render(VkCommandBuffer cmdBuff);
	
	void destroy();
	
private:
	VKLDevice* m_device = NULL;
	
	VKLBuffer* m_vertBuffer = NULL;
	VKLBuffer* m_uniformBuffer = NULL;
	
	VKLShader* m_shader = NULL;
	VKLUniformObject* m_uniform = NULL;
	VKLPipeline* m_pipeline = NULL;
	
};

#endif /* Cursor_hpp */
