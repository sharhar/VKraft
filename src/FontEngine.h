//
//  FontEngine.hpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#ifndef FontEngine_h
#define FontEngine_h

#include <VKL/VKL.h>
#include <string>

class FontEngine {
public:
	static void init(VKLDevice* device, VKLFrameBuffer* framebuffer);
	static void updateProjection(int width, int height);
	static void render(VkCommandBuffer cmdBuffer);
	static void setText(const std::string& str);
	static void setCoords(float xPos, float yPos, float size);
	static void destroy();
	
private:
	static int m_charNum;
	
	static VKLDevice* m_device;
	static VKLBuffer* m_vertBuffer;
	static VKLBuffer* m_instanceBuffer;
	static VKLBuffer* m_uniformBuffer;
	static VKLShader* m_shader;
	static VKLUniformObject* m_uniform;
	static VKLTexture* m_texture;
	static VKLPipeline* m_pipeline;
};

#endif /* FontEngine_hpp */
