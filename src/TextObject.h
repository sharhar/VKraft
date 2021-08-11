//
//  FontEngine.hpp
//  VKraft
//
//  Created by Shahar Sandhaus on 6/11/21.
//

#ifndef TextObject_h
#define TextObject_h

#include <VKL/VKL.h>
#include <string>

class TextObject {
public:
	static void init(const VKLDevice* device, const VKLQueue* transferQueue, const VKLRenderTarget* renderTarget);
	void updateProjection();
	static void destroy();
	
	TextObject(int maxCharNum);
	void render(VkCommandBuffer cmdBuffer);
	void setText(const std::string& str);
	void setCoords(float xPos, float yPos, float size);
	
private:
	static const VKLDevice* m_device;
	static const VKLRenderTarget* m_renderTarget;

	static VKLBuffer m_vertBuffer;
	static VKLShader m_shader;
	//VKLUniformObject* m_uniform;
	static VKLImage m_texture;
	static VKLPipeline m_pipeline;
	
	VKLBuffer* m_instanceBuffer;
	VKLBuffer* m_uniformBuffer;
	int m_charNum;
};

#endif /* TextObject_h */
