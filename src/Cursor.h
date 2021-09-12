#ifndef Cursor_h
#define Cursor_h

#include "Base.h"

class Cursor {
public:
	Cursor(Application* application);
	void bindInputAttachment(const VKLImageView* view);
	void render(VKLCommandBuffer* cmdBuffer);
	void destroy();
private:
	Application* m_application;
	
	VKLBuffer m_vertBuffer;
	VKLPipelineLayout m_layout;
	
	VKLPipeline m_pipeline;
	
	float m_screenSize[2];
	
	VKLDescriptorSet* m_descriptorSet;
	
};

#endif /* Cursor_h */
