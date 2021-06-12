//
//  FontEngine.hpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#ifndef FontEngine_h
#define FontEngine_h

#include <VKL/VKL.h>

class FontEngine {
public:
	static void init(VKLDevice* device, VKLSwapChain* swapChain);
	static void updateProjection(int width, int height);
	
	static void destroy();
	
private:
	static VKLDevice* m_device;
	
	static VKLBuffer* m_uniformBuffer;
	static VKLShader* m_shader;
	static VKLUniformObject* m_uniform;
	static VKLTexture* m_texture;
	static VKLPipeline* m_pipeline;
};

#endif /* FontEngine_hpp */
