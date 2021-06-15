//
//  BG.h
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#ifndef BG_h
#define BG_h

#include <VKL/VKL.h>

class BG {
public:
	static void init(VKLDevice* device, VKLSwapChain* swapChain, VKLFrameBuffer* framebuffer);
	static void rebuildPipeline(VKLSwapChain* swapChain, VKLFrameBuffer* framebuffer);
	static void render(VkCommandBuffer cmdBuff);
	static void destroy();
	
private:
	static VKLDevice* m_device;
	static VKLBuffer* m_vertBuffer;
	static VKLShader* m_shader;
	static VKLUniformObject* m_uniform;
	static VKLPipeline* m_pipeline;
};

#endif /* BG_h */
