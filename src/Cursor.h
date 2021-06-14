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
	static void init(VKLDevice* device, VKLSwapChain* swapChain, VKLFrameBuffer* framebuffer);
	static void updateProjection(int width, int height);
	static void render(VkCommandBuffer cmdBuff);
	static void destroy();
private:
	static VKLDevice* m_device;
	
	static VKLBuffer* m_vertBuffer;
	static VKLBuffer* m_uniformBuffer;
	static VKLShader* m_shader;
	static VKLUniformObject* m_uniform;
	static VKLPipeline* m_pipeline;
};

#endif /* Cursor_h */
