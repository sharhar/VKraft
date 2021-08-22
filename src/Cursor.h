#ifndef Cursor_h
#define Cursor_h

#include "Base.h"

class Cursor {
public:
	Cursor(Application* application);
	void updateProjection(int width, int height);
	void render(VKLCommandBuffer* cmdBuffer);
	void destroy();
private:
	Application* m_application;
	
	VKLBuffer m_vertBuffer;
	VKLShader m_shader;
	
	VKLPipeline m_pipeline;
	
	float m_screenSize[2];
	
	VKLDescriptorSet* m_descriptorSet;
	
};

#endif /* Cursor_h */
